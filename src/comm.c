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
#define COMPRESS_BUF_SIZE 1024

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

/*
 * OS-dependent local functions.
 */
void	game_loop		args( ( ) );
int	init_socket		args( ( int port ) );
void	new_descriptor		args( ( int new_desc ) );
bool	read_from_descriptor	args( ( DESCRIPTOR_DATA *d ) );


/*
 * Other local functions (OS-independent).
 */
void output_to_descriptor(DESCRIPTOR_DATA *d, const char *txt);
void write_to_buffer_str(DESCRIPTOR_DATA *d, const char *txt);
static void append_str(char **buf, size_t *len, size_t *cap, const char *src);
char *wrap_text_smart(const char *txt, int width);
bool is_structured_line(const char *line);
bool looks_preformatted(const char *txt);
size_t visible_length(const char *txt);
size_t visible_length_ex(const char *txt, int flags);
char *wrap_text_ex(const char *txt, int width, int flags, int indent);
bool process_compressed(DESCRIPTOR_DATA *d);
bool	check_parse_name	args( ( char *name ) );
short	check_reconnect		args( ( DESCRIPTOR_DATA *d, char *name,
				    bool fConn ) );
short	check_playing		args( ( DESCRIPTOR_DATA *d, char *name, bool kick ) );
bool	check_multi		args( ( DESCRIPTOR_DATA *d, char *name ) );
int	main			args( ( int argc, char **argv ) );
void	nanny			args( ( DESCRIPTOR_DATA *d, char *argument ) );
bool	flush_buffer		args( ( DESCRIPTOR_DATA *d, bool fPrompt ) );
void	read_from_buffer	args( ( DESCRIPTOR_DATA *d ) );
void	stop_idling		args( ( CHAR_DATA *ch ) );
void	free_desc		args( ( DESCRIPTOR_DATA *d ) );
void	display_prompt		args( ( DESCRIPTOR_DATA *d ) );
int	make_color_sequence	args( ( const char *col, char *buf,
					DESCRIPTOR_DATA *d ) );
void	set_pager_input		args( ( DESCRIPTOR_DATA *d,
					char *argument ) );
bool	pager_output		args( ( DESCRIPTOR_DATA *d ) );



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

    num_descriptors		= 0;
    first_descriptor		= NULL;
    last_descriptor		= NULL;
    sysdata.NO_NAME_RESOLVING	= TRUE;
    sysdata.WAIT_FOR_AUTH	= TRUE;

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
    reboot_check(mktime(new_boot_time));
    new_boot_time_t = mktime(new_boot_time);

    /* Set reboot time string for do_time */
    get_reboot_string();

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
    boot_db( );
    log_string("Initializing socket");
    control  = init_socket( port   );
    control2 = init_socket( port+1 );
    conclient= init_socket( port+10);
    conjava  = init_socket( port+20);
    log_printf( "Rise in Power ready on port %d.", port );
    bootup = FALSE;
    game_loop( );
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
static void caught_alarm()
{
    char buf[MAX_STRING_LENGTH];
    bug( "ALARM CLOCK!" );
    SPRINTF( buf, "Alas, the hideous mandalorian entity known only as 'Lag' rises once more!\n" );
    echo_to_all( AT_IMMORT, buf, ECHOTAR_ALL );
    if ( newdesc )
    {
	FD_CLR( newdesc, &in_set );
	FD_CLR( newdesc, &out_set );
	log_string( "clearing newdesc" );
    }
    game_loop( );
    close( control );

    log_string( "Normal termination of game." );
    exit( 0 );
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


void accept_new( int ctrl )
{
	static struct timeval null_time;
	DESCRIPTOR_DATA *d;
	/* int maxdesc; Moved up for use with id.c as extern */

#if defined(MALLOC_DEBUG)
	if ( malloc_verify( ) != 1 )
	    abort( );
#endif

	/*
	 * Poll all active descriptors.
	 */
	FD_ZERO( &in_set  );
	FD_ZERO( &out_set );
	FD_ZERO( &exc_set );
	FD_SET( ctrl, &in_set );
	maxdesc	= ctrl;
	newdesc = 0;
	for ( d = first_descriptor; d; d = d_next )
	{
        d_next = d->next;
        if (d->descriptor < 0)
            continue;

        maxdesc = UMAX( maxdesc, d->descriptor );
	    FD_SET( d->descriptor, &in_set  );
	    FD_SET( d->descriptor, &out_set );
	    FD_SET( d->descriptor, &exc_set );
	    if (d->auth_fd != -1)
	    {
		maxdesc = UMAX( maxdesc, d->auth_fd );
		FD_SET(d->auth_fd, &in_set);
		if (IS_SET(d->auth_state, FLAG_WRAUTH))
		  FD_SET(d->auth_fd, &out_set);
	    }
	    if ( d == last_descriptor )
	      break;
	}

	if ( select( maxdesc+1, &in_set, &out_set, &exc_set, &null_time ) < 0 )
	{
	    perror( "accept_new: select: poll" );
	    exit( 1 );
	}

	if ( FD_ISSET( ctrl, &exc_set ) )
	{
	    bug( "Exception raise on controlling descriptor %d", ctrl );
	    FD_CLR( ctrl, &in_set );
	    FD_CLR( ctrl, &out_set );
	}
	else
	if ( FD_ISSET( ctrl, &in_set ) )
	{
	    newdesc = ctrl;
	    new_descriptor( newdesc );
	}
}

void game_loop( )
{
    struct timeval	  last_time;
    char cmdline[MAX_INPUT_LENGTH];
    DESCRIPTOR_DATA *d;
//  AREA_DATA *pArea;

/*  time_t	last_check = 0;  */

    signal( SIGPIPE, SIG_IGN );
    signal( SIGALRM, (void (*) (int) )caught_alarm );
    /* signal( SIGSEGV, SegVio ); */
    gettimeofday( &last_time, NULL );
    current_time = (time_t) last_time.tv_sec;
    

    /* Main loop */
    while ( !mud_down )
    {
	accept_new( control  );
	accept_new( control2 );
	accept_new( conclient);
	accept_new( conjava  );
        cron();
	/*
	 * Kick out descriptors with raised exceptions
	 * or have been idle, then check for input.
	 */
	for ( d = first_descriptor; d; d = d_next )
	{
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
	    ( d->idle > 7200 ) && !IS_SET(d->character->act, PLR_AFK))		  /* 30 minutes  */
	    {
	      if( (d->character && d->character->in_room) ? d->character->top_level <= LEVEL_IMMORTAL : FALSE)
	      {
            output_to_descriptor( d,
            "Idle 30 Minutes. Activating AFK Flag\n");
		SET_BIT(d->character->act, PLR_AFK);
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

        while ( d->inbuf_len > 0 )
        {
            /* Stop if a command is already pending */
            if ( d->incomm[0] != '\0' )
                break;

            read_from_buffer( d );

            /* No full command ready */
            if ( d->incomm[0] == '\0' )
                break;

            d->fcommand = TRUE;
            stop_idling( d->character );

            SPRINTF( cmdline, "%s", d->incomm );
            d->incomm[0] = '\0';

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
	update_handler( );

	/*
	 * Check REQUESTS pipe
	 */
        check_requests( );

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
		    perror( "game_loop: select: stall" );
		    exit( 1 );
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


void new_descriptor( int new_desc )
{
    char buf[MAX_STRING_LENGTH];
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
    if ( check_bad_desc( new_desc ) )
    {
      set_alarm( 0 );
      return;
    }
#if !defined(FNDELAY)
#define FNDELAY O_NDELAY
#endif

    set_alarm( 20 );
    if ( fcntl( desc, F_SETFL, FNDELAY ) == -1 )
    {
	perror( "New_descriptor: fcntl: FNDELAY" );
	set_alarm( 0 );
	return;
    }
    if ( check_bad_desc( new_desc ) )
      return;

    CREATE( dnew, DESCRIPTOR_DATA, 1 );
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
    dnew->prevcolor	= 0x07;
    dnew->original      = NULL;
    dnew->character     = NULL;
    dnew->telnet_pos = 0;
    dnew->pagepoint = PAGEPOINT_NULL;

    CREATE( dnew->outbuf, char, dnew->outsize );

    SPRINTF( buf, "%s", inet_ntoa( sock.sin_addr ) );

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

 /* Noresolve now does something useful - DV - Stuff for dontresolve. - Ulysses */
     if ( !sysdata.NO_NAME_RESOLVING && !check_dont_resolve(buf) )
         from = gethostbyaddr( (char *) &sock.sin_addr,
     	  	sizeof(sock.sin_addr), AF_INET );
     else
         from = NULL;
    hostname = STRALLOC( (char *)( from ? from->h_name : "") );

/*	
    if ( !str_prefix( dnew->host, "172.1" ) )
    {
//    log_string_plus( "AOL Ping!", LOG_COMM, sysdata.log_level );
      SPRINTF( buf, inet_ntoa( sock.sin_addr ) );
      log_printf_plus( LOG_COMM, sysdata.log_level, "Sock.sinaddr: AOL Ping! %s, port %hd.",
		buf, dnew->port );
      
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
	    set_alarm( 0 );
	    return;
	}
    }

    SPRINTF( buf, "%s", inet_ntoa( sock.sin_addr ) );
    log_printf_plus( LOG_COMM, sysdata.log_level, "Sock.sinaddr:  %s, port %d.",	buf, dnew->port );

    if ( !sysdata.NO_NAME_RESOLVING )
    {
       STRFREE ( dnew->host);
       dnew->host = STRALLOC( (char *)( from ? from->h_name : buf) );
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

    LINK( dnew, first_descriptor, last_descriptor, next, prev );

#ifdef MCCP
    output_to_descriptor(dnew, (const char *)eor_on_str);
    output_to_descriptor(dnew, (const char *)compress2_on_str);
    output_to_descriptor(dnew, (const char *)compress_on_str);
#endif

    const unsigned char will_naws[] = { IAC, DO, TELOPT_NAWS };
    write_to_buffer(dnew,(const char *)(will_naws), 3);

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

    if ( ++num_descriptors > sysdata.maxplayers )
    	sysdata.maxplayers = num_descriptors;
    if ( sysdata.maxplayers > sysdata.alltimemax )
    {
      if ( sysdata.time_of_max )
        DISPOSE(sysdata.time_of_max);
      SPRINTF(buf, "%24.24s", ctime(&current_time));
      sysdata.time_of_max = str_dup(buf);
      sysdata.alltimemax = sysdata.maxplayers;
      SPRINTF( buf, "Broke all-time maximum player record: %d", sysdata.alltimemax );
      log_string_plus( buf, LOG_COMM, sysdata.log_level );
      to_channel( buf, CHANNEL_MONITOR, "Monitor", LEVEL_IMMORTAL );
      save_sysdata( sysdata );
    }
    set_alarm(0);
    return;
}

void free_desc( DESCRIPTOR_DATA *d )
{
    close( d->descriptor );
    d->descriptor = -1;
    STRFREE( d->host );
    DISPOSE( d->outbuf );
    STRFREE( d->user );    /* identd */


#ifdef MCCP
    compressEnd(d);
    DISPOSE( d->out_compress_buf );
#endif

    if ( d->pagebuf )
	    DISPOSE( d->pagebuf );
    --num_descriptors;
    return;
}

void close_socket( DESCRIPTOR_DATA *dclose, bool force )
{
    CHAR_DATA *ch;
    DESCRIPTOR_DATA *d;
    bool DoNotUnlink = FALSE;

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
	log_printf_plus( LOG_COMM, UMAX( sysdata.log_level, ch->top_level ), "Closing link to %s.", ch->name );
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

int telnet_process( DESCRIPTOR_DATA *d, const unsigned char *in, int in_len, unsigned char *out, int out_max )
{
    int i, out_len = 0;

    for (i = 0; i < in_len; i++)
    {
        unsigned char c = in[i];
        bool process_char = true;

        switch (d->telstate)
        {
            case TS_DATA:
                if (c == IAC)
                {
                    d->telstate = TS_IAC;
                    process_char = false;
                }
                break;

            case TS_IAC:
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
                        if (!d->naws_enabled)
                            d->telstate = TS_DATA;
                        else
                            d->telstate = TS_SB;

                        process_char = false;
                        break;

                    default:
                        d->telstate = TS_DATA;
                        process_char = false;
                        break;
                }
                break;

            case TS_WILL:
                if (c == TELOPT_NAWS)
                {
                    const unsigned char do_naws[] = { IAC, DO, TELOPT_NAWS };
                    write_to_buffer(d, (char *)do_naws, 3);
                    flush_buffer(d, FALSE);

                    d->naws_enabled = true;

                    bug("NAWS enabled (client WILL)");
                }

                d->telstate = TS_DATA;
                process_char = false;
                break;

            case TS_WONT:
                d->telstate = TS_DATA;
                process_char = false;
                break;

            case TS_DO:
            #ifdef MCCP
                if (c == TELOPT_COMPRESS2)
                {
                    const unsigned char will_comp[] = { IAC, WILL, TELOPT_COMPRESS2 };
                    write_to_buffer(d, (char *)will_comp, 3);
                    flush_buffer(d, FALSE);

                    /*
                    * DO NOT start compression yet
                    * Wait for client to acknowledge
                    */
                    d->mccp_pending = 1;  /* NEW STATE: awaiting confirmation */

                    bug("MCCP: client DO COMPRESS2 -> will start compression when playing");
                }
            #endif
                if (c == TELOPT_NAWS)
                {
                    const unsigned char do_naws[] = { IAC, WILL, TELOPT_NAWS };
                    write_to_buffer(d, (char *)do_naws, 3);
                    flush_buffer(d, FALSE);

                    d->naws_enabled = true;
                }

                d->telstate = TS_DATA;
                process_char = false;
                break;

            case TS_DONT:
#ifdef MCCP
                if (c == TELOPT_COMPRESS && d->compressing == TELOPT_COMPRESS)
                    compressEnd(d);
                else if (c == TELOPT_COMPRESS2 && d->compressing == TELOPT_COMPRESS2)
                    compressEnd(d);
#endif
                d->telstate = TS_DATA;
                process_char = false;
                break;

            case TS_SB:
                d->sb_option = c;
                d->sb_len = 0;

                if (c == TELOPT_NAWS && d->naws_enabled)
                {
                    d->telstate = TS_SB_DATA;
                }
                else
                {
                    /* INVALID SB → DROP */
                    d->telstate = TS_DATA;
                    d->sb_len = 0;
                }

                process_char = false;
                break;

            case TS_SB_DATA:
                if (c == IAC)
                {
                    d->telstate = TS_SB_IAC;
                    process_char = false;
                    break;
                }

                if (d->sb_option != TELOPT_NAWS)
                {
                    process_char = false;
                    break;
                }

                if (d->sb_len < 4)
                    d->sb_buf[d->sb_len++] = c;

                process_char = false;
                break;

            case TS_SB_IAC:
                if (c == SE)
                {
                    if (d->sb_option == TELOPT_NAWS && d->sb_len == 4)
                    {
                        int width  = (d->sb_buf[0] << 8) | d->sb_buf[1];
                        int height = (d->sb_buf[2] << 8) | d->sb_buf[3];

                        if (width < 20 || width > 500)
                            width = 80;

                        d->term_width  = width;
                        d->term_height = height;
                    }

                    d->telstate = TS_DATA;
                    d->sb_len = 0;
                }
                else if (c == IAC)
                {
                    if (d->sb_len < 4)
                        d->sb_buf[d->sb_len++] = IAC;

                    d->telstate = TS_SB_DATA;
                }
                else
                {
                    /* INVALID SB TERMINATION → RESET */
                    d->telstate = TS_DATA;
                    d->sb_len = 0;
                }

                process_char = false;
                break;
        }

        /* Emit only real data bytes */
        if (process_char)
        {
            if (out_len < out_max)
                out[out_len++] = c;
        }
    }

    return out_len;
}

bool read_from_descriptor( DESCRIPTOR_DATA *d )
{
    unsigned int iStart;

    /* Hold horses if pending command already. */
    if ( d->incomm[0] != '\0' )
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
            if ( d->inbuf[iStart-1] == '\n' || d->inbuf[iStart-1] == '\r' )
            break;
        }
        else if ( nRead == 0 )
        {
            log_string_plus( "EOF encountered on read.", LOG_COMM, sysdata.log_level );
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



/*
 * Transfer one line from input buffer to input line.
 */
void read_from_buffer( DESCRIPTOR_DATA *d )
{
    int i, k;

    if ( d->incomm[0] != '\0' )
        return;

    /*
     * ============================================================
     * STEP 1: PROCESS ONLY NEW DATA (CRITICAL FIX)
     * ============================================================
     *
     * BEFORE:
     *   You processed entire inbuf every time ❌
     *
     * NOW:
     *   Only process unprocessed bytes ✅
     */
    unsigned char clean[MAX_INBUF_SIZE];

    int new_bytes = d->inbuf_len - d->telnet_pos;  /* NEW */

    if ( new_bytes <= 0 )
        return;

    int clean_len = telnet_process(
        d,
        (unsigned char *)d->inbuf + d->telnet_pos,  /* NEW OFFSET */
        new_bytes,
        clean,
        sizeof(clean)
    );

    d->telnet_pos += new_bytes;  /* MARK AS PROCESSED */

//  bug("TELNET: clean_len=%d new_bytes=%d inbuf_len=%d",
//      clean_len, new_bytes, d->inbuf_len);

    /*
     * ============================================================
     * STEP 2: FIND NEWLINE
     * ============================================================
     */
    int line_end = -1;

    for ( i = 0; i < clean_len; i++ )
    {
        if ( clean[i] == '\n' || clean[i] == '\r' )
        {
            line_end = i;
            break;
        }
    }

    if ( line_end == -1 )
        return;

    /*
     * ============================================================
     * STEP 3: BUILD COMMAND
     * ============================================================
     */
    for ( i = 0, k = 0; i < line_end; i++ )
    {
        unsigned char c = clean[i];

        if ( k >= 254 )
        {
            output_to_descriptor( d, "Line too long.\n" );
            break;
        }

        if ( (c == '\b' || c == 127) && k > 0 )
            --k;
        else if ( isprint(c) )
            d->incomm[k++] = (char)c;
    }

    if ( k == 0 )
        d->incomm[k++] = ' ';

    d->incomm[k] = '\0';

    /*
     * ============================================================
     * STEP 4: COMMAND RATE LIMIT
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
     * STEP 5: HISTORY (!! support)
     * ============================================================
     */
    if ( k > 1 || d->incomm[0] == '!' )
    {
        if ( d->incomm[0] != '!' && strcmp( d->incomm, d->inlast ) )
            d->repeat = 0;
        else if ( ++d->repeat >= 100 )
            output_to_descriptor( d,
                "\n*** PUT A LID ON IT!!! ***\n" );
    }

    if ( d->incomm[0] == '!' )
        SPRINTF( d->incomm, "%s", d->inlast );
    else
        SPRINTF( d->inlast, "%s", d->incomm );

    /*
     * ============================================================
     * STEP 6: SHIFT RAW BUFFER (FIXED)
     * ============================================================
     *
     * IMPORTANT:
     * - We must ALSO reset telnet_pos
     * - Otherwise it points into garbage after memmove
     */
    int shift = 0;
    int seen_newline = 0;

    for ( i = 0; i < d->inbuf_len; i++ )
    {
        if (!seen_newline)
        {
            if ( d->inbuf[i] == '\n' || d->inbuf[i] == '\r' )
                seen_newline = 1;
        }
        else
        {
            if ( d->inbuf[i] != '\n' && d->inbuf[i] != '\r' )
            {
                shift = i;
                break;
            }
        }
    }

    if ( i >= d->inbuf_len )
        shift = d->inbuf_len;

    int remaining = d->inbuf_len - shift;

    if ( remaining > 0 )
        memmove( d->inbuf, d->inbuf + shift, remaining );

    d->inbuf_len = remaining;

    /*
     * 🔥 CRITICAL FIX:
     * Reset telnet position after buffer shift
     */
    d->telnet_pos = 0;

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
    
    ch = d->original ? d->original : d->character;
    if( ch && ch->fighting && ch->fighting->who )
         show_condition( ch, ch->fighting->who );


    /*
     * Bust a prompt.
     */
    if ( fPrompt && !mud_down && d->connected == CON_PLAYING )
    {
	ch = d->original ? d->original : d->character;
	if ( IS_SET(ch->act, PLR_BLANK) )
	    output_to_descriptor( d, "\n" );

	if ( IS_SET(ch->act, PLR_PROMPT) )
	    display_prompt(d);
	if ( IS_SET(ch->act, PLR_TELNET_GA) )
	    output_to_descriptor( d, (const char *) go_ahead_str );
    }

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
        d->out_compress->next_in  = (unsigned char *)d->outbuf;
        d->out_compress->avail_in = d->outtop;

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

        d->outtop = 0;

        return process_compressed(d);
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
                break;

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

    return TRUE;
}

void output_to_descriptor(DESCRIPTOR_DATA *d, const char *txt)
{
    char *wrapped;

    if (!d || !txt)
        return;

    wrapped = wrap_text_smart(txt, d->term_width);

fprintf(stderr, "AFTER WRAP:\n");
for (char *p = wrapped; *p; p++)
    fprintf(stderr, "%02X ", (unsigned char)*p);
fprintf(stderr, "\n");

    if (!wrapped)
        return;

    write_to_buffer_str(d, wrapped);

    /* 🔹 CRITICAL: free heap memory */
    free(wrapped);

    return;
}


void write_to_buffer_str(DESCRIPTOR_DATA *d, const char *txt)
{
    write_to_buffer(d, txt, strnlen(txt, MAX_STRING_LENGTH));
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
    if ( !d->outbuf )
    	return;

    // Find length in case caller didn't.
    if ( length <= 0 )
    {
        if ( !txt )
            return;
        length = strlen(txt);  /* fallback only */
    }

    if ( length <= 0 )
        return;

    // Initial \n\r if needed.
    if ( d->outtop == 0 && !d->fcommand )
    {
        d->outbuf[0]	= '\r';
        d->outbuf[1]	= '\n';
        d->outtop	= 2;
    }

    /*
     * HARD CAP (slow client protection)
     * WHY:
     * - Prevent infinite growth if client isn't reading
     * - Protects server from output flooding
     */
    if ( d->outtop + length > 50000 )  /* tune this */
    {
        if ( count == 0 )
        {
            bug("write_to_buffer: overflow (%s) [%d bytes queued]",
                d->character ? d->character->name : d->host,
                d->outtop);
            count++;
            close_socket(d, TRUE);
        }
        count = 0;
        return;
    }

    // Expand the buffer as needed.
    while ( d->outtop + (unsigned int ) length >= d->outsize )
    {
        if (d->outsize > 65536)
        {
            /* empty buffer */
            d->outtop = 0;
            bug("write_to_buffer: max buffer exceeded (%s)", d->character ? d->character->name : "???" );
            close_socket(d, TRUE);
            return;
        }
        d->outsize *= 2;
        RECREATE( d->outbuf, char, d->outsize );
    }

        #define COPY_WITH_NEWLINES(src, len)                                     \
    do {                                                                     \
        const char *p = (src);                                               \
        const char *end = (src) + (len);                                     \
                                                                             \
        while (p < end)                                                      \
        {                                                                    \
            if (d->outtop + 2 >= d->outsize)                                 \
            {                                                                \
                d->outsize *= 2;                                             \
                RECREATE(d->outbuf, char, d->outsize);                       \
            }                                                                \
                                                                             \
            if (*p == '\r')                                                  \
            {                                                                \
                if ((p + 1) < end && *(p + 1) == '\n')                       \
                {                                                            \
                    d->outbuf[d->outtop++] = '\r';                           \
                    d->outbuf[d->outtop++] = '\n';                           \
                    p += 2;                                                  \
                    continue;                                                \
                }                                                            \
                                                                             \
                d->outbuf[d->outtop++] = '\r';                               \
                d->outbuf[d->outtop++] = '\n';                               \
                p++;                                                         \
                continue;                                                    \
            }                                                                \
                                                                             \
            if (*p == '\n')                                                  \
            {                                                                \
                if ((p + 1) < end && *(p + 1) == '\r')                       \
                    p++; /* skip bad order */                                \
                                                                             \
                d->outbuf[d->outtop++] = '\r';                               \
                d->outbuf[d->outtop++] = '\n';                               \
                p++;                                                         \
                continue;                                                    \
            }                                                                \
                                                                             \
            d->outbuf[d->outtop++] = *p++;                                   \
        }                                                                    \
    } while (0)

    /* =========================
    * COLOR-AWARE COPY
    * ========================= */
    const char *ptr = txt;         
    const char *end = txt + length;
    char colbuf[64];
    int ln;

    while ( ptr < end )                         /* CHANGED */
    {
        const char *colstr = NULL;              /* CHANGED */

        /* CHANGED: manual scan instead of strpbrk */
        for ( const char *p = ptr; p < end; ++p )
        {
            if ( *p == '&' || *p == '^' )
            {
                colstr = p;
                break;
            }
        }

        if ( !colstr )                          /* CHANGED */
            break;

        /* copy text before color */
        size_t seglen = (size_t)(colstr - ptr); /* CHANGED */

        if ( seglen > 0 )
        {
            COPY_WITH_NEWLINES(ptr, seglen);
        }

        /* CHANGED: bounds check for &X */
        if ( colstr + 1 >= end )
        {
            ptr = colstr;
            break;
        }

        /* convert color */
        ln = make_color_sequence( colstr, colbuf, d );

        if ( ln < 0 )
        {
            ptr = colstr + 1;                  /* CHANGED */
            continue;
        }

        if ( ln > 0 )
        {
            while ( d->outtop + (size_t)ln >= d->outsize )
            {
                d->outsize *= 2;
                RECREATE( d->outbuf, char, d->outsize );
            }

            memcpy( d->outbuf + d->outtop, colbuf, (size_t)ln );
            d->outtop += (size_t)ln;
        }

        ptr = colstr + 2;                      /* CHANGED */
    }

    /* =========================
     * CHANGED: remainder copy (length-safe)
     * ========================= */
    if ( ptr < end )
    {
        size_t rem = (size_t)(end - ptr);

        COPY_WITH_NEWLINES(ptr, rem);  /* NEW */
    }

    d->outbuf[d->outtop] = '\0';
    return;
}


#ifdef MCCP
#define COMPRESS_BUF_SIZE 1024

bool write_to_descriptor_depreciated(int desc, char *txt, int length)
{
    bug("write_to_descriptor: deprecated path used!");
    return FALSE;
}
#else

/*
 * Lowest level output function.
 * Write a block of text to the file descriptor.
 * If this gives errors on very long blocks (like 'ofind all'),
 *   try lowering the max block size.
 */
bool write_to_descriptor_depreciated( int desc, char *txt, int length )
{
{
    bug("write_to_descriptor: deprecated path used!");
    return FALSE;
}
}

#endif

void show_title( DESCRIPTOR_DATA *d )
{
    CHAR_DATA *ch;

    ch = d->character;

    if ( !IS_SET( ch->pcdata->flags, PCFLAG_NOINTRO ) )
    {
	if (IS_SET(ch->act, PLR_ANSI))
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
void nanny( DESCRIPTOR_DATA *d, char *argument )
{
/*	extern int lang_array[];
	extern char *lang_names[];*/
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char arg[MAX_STRING_LENGTH];
    CHAR_DATA *ch;
    char *pwdnew;
    char *p;
    int iRace, iClass, halfmax;
    BAN_DATA *pban;
/*    int iLang;*/
    bool fOld;
    short chk;

    while ( isspace(*argument) )
	argument++;

    ch = d->character;

    switch ( d->connected )
    {

    default:
	bug( "Nanny: bad d->connected %d.", d->connected );
	close_socket( d, TRUE );
	return;

    case CON_GET_NAME:
	if ( argument[0] == '\0' )
	{
	    close_socket( d, FALSE );
	    return;
	}

	argument[0] = UPPER(argument[0]);
	if ( !check_parse_name( argument ) )
	{
	    output_to_descriptor( d, "Illegal name, try another.\nName: " );
	    return;
	}

  if ( !str_cmp( argument, "New" ) )
	{
	    if (d->newstate == 0)
	    {
              /* New player */
              /* Don't allow new players if DENY_NEW_PLAYERS is true */
      	      if (sysdata.DENY_NEW_PLAYERS == TRUE)
      	      {
                SPRINTF( buf, "The mud is currently preparing for a reboot.\n" );
                output_to_descriptor( d, buf );
                SPRINTF( buf, "New players are not accepted during this time.\n" );
                output_to_descriptor( d, buf );
                SPRINTF( buf, "Please try again in a few minutes.\n" );
                output_to_descriptor( d, buf );
                close_socket( d, FALSE );
              }
              SPRINTF( buf, "\nChoosing a name is one of the most important parts of this game...\n"
              			"Make sure to pick a name appropriate to the character you are going\n"
               			"to role play, and be sure that it suits our Star Wars theme.\n"
               			"If the name you select is not acceptable, you will be asked to choose\n"
               			"another one.\n\nPlease choose a name for your character: ");
              output_to_descriptor( d, buf );
	      d->newstate++;
	      d->connected = CON_GET_NAME;
	      return;
	    }
	    else
   	    {
	      output_to_descriptor(d, "Illegal name, try another.\nName: " );
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
	    log_printf( "Bad player file %s@%s.", argument, d->host );
	    output_to_descriptor( d, "Your playerfile is corrupt...Please notify the admins.\n" );
	    close_socket( d, FALSE );
	    return;
	}
	ch   = d->character;

        for ( pban = first_ban; pban; pban = pban->next )
        {
	  if ( 
	  ( !str_prefix( pban->name, d->host ) 
	    || !str_suffix( pban->name, d->host ) )
	  && pban->level >= ch->top_level )
          {
            output_to_descriptor( d,
              "Your site has been banned from this Mud.\n" );
            close_socket( d, FALSE );
	    return;
	  }
        }
	if ( IS_SET(ch->act, PLR_DENY) )
	{
	    log_printf_plus( LOG_COMM, sysdata.log_level, "Denying access to %s@%s.", argument, d->host );
	    if (d->newstate != 0)
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
	    if ( wizlock && !IS_IMMORTAL(ch) )
	    {
		output_to_descriptor( d, "The game is wizlocked.  Only immortals can connect now.\n" );
		output_to_descriptor( d, "Please try back later.\n" );
		close_socket( d, FALSE );
		return;
	    }
	}

	if ( fOld )
	{
	    if (d->newstate != 0)
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
     if (check_bad_name(ch->name)) {
       output_to_descriptor( d, "\nThat name is unacceptable, please choose another.\n");
       output_to_descriptor( d, "Name: ");
       d->connected = CON_GET_NAME;
       return;
     }
            output_to_descriptor( d, "\nI don't recognize your name, you must be new here.\n\n" );
            SPRINTF( buf, "Did I get that right, %s (Y/N)? ", argument );
            output_to_descriptor( d, buf );
            d->connected = CON_CONFIRM_NEW_NAME;
	    return;
	}
	break;

    case CON_GET_OLD_PASSWORD:
	output_to_descriptor( d, "\n" );
//New SHA-256 password checking - AI/DV 3-12-26
  char pwdhash[65];
  sha256_hash(argument, pwdhash);

  if (strcmp(pwdhash, ch->pcdata->pwd))
  {
      output_to_descriptor(d, "Wrong password.\n");
	    // clear descriptor pointer to get rid of bug message in log      
      d->character->desc = NULL;
      close_socket(d, FALSE);
      return;
  }
/*
	if ( strcmp( crypt( argument, ch->pcdata->pwd ), ch->pcdata->pwd ) )
	{
	    output_to_descriptor( d, "Wrong password.\n" );
	    // clear descriptor pointer to get rid of bug message in log
	    d->character->desc = NULL;
	    close_socket( d, FALSE );
	    return;
	}
*/
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
        
        if ( check_multi( d , ch->name  ) )
        {
            close_socket( d, FALSE );
            return;
        }
        
	SPRINTF( buf, "%s", ch->name );
	d->character->desc = NULL;
	free_char( d->character );
	fOld = load_char_obj( d, buf, FALSE );
	ch = d->character;

	if ( ch->top_level < LEVEL_DEMI )
	{
	  log_printf_plus( LOG_COMM, sysdata.log_level, "%s@%s(%s) has connected.", ch->name, d->host, d->user );    
	}
	else
	  log_printf_plus( LOG_COMM, ch->top_level, "%s@%s(%s) has connected.", ch->name, d->host, d->user );    

	show_title(d);
	if ( ch->pcdata->area )
		do_loadarea (ch , "" );
	
	
	break;

    case CON_CONFIRM_NEW_NAME:
	switch ( *argument )
	{
	case 'y': case 'Y':
	    SPRINTF( buf, "\nMake sure to use a password that won't be easily guessed by someone else."
	    		  "\nPick a good password for %s: %s",
		ch->name, echo_off_str );
	    output_to_descriptor( d, buf );
	    d->connected = CON_GET_NEW_PASSWORD;
	    break;

	case 'n': case 'N':
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
	break;

    case CON_GET_NEW_PASSWORD:
	output_to_descriptor( d, "\n" );

	if ( visible_length(argument) < 5 )
	{
	    output_to_descriptor( d,
		"Password must be at least five characters long.\nPassword: " );
	    return;
	}

//	pwdnew = crypt( argument, ch->name );
// New SHA-256 password hashing - AI/DV 3-12-26
  char pwdnewhash[65];
  sha256_hash(argument, pwdnewhash);
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

	DISPOSE( ch->pcdata->pwd );
	ch->pcdata->pwd	= str_dup( pwdnew );
	output_to_descriptor( d, "\nPlease retype the password to confirm: " );
	d->connected = CON_CONFIRM_NEW_PASSWORD;
	break;

    case CON_CONFIRM_NEW_PASSWORD:
	output_to_descriptor( d, "\n" );
  
// New SHA-256 password checking - AI/DV 3-12-26
sha256_hash(argument, pwdnewhash);

for (p = pwdnewhash; *p != '\0'; p++)
{
    if (*p == '~')
    {
        output_to_descriptor(d,
            "New password not acceptable, try again.\nPassword: " );
        return;
    }
}

DISPOSE(ch->pcdata->pwd);
ch->pcdata->pwd = str_dup(pwdnewhash);
/*

	if ( strcmp( crypt( argument, ch->pcdata->pwd ), ch->pcdata->pwd ) )
	{
	    output_to_descriptor( d, "Passwords don't match.\nRetype password: " );
	    d->connected = CON_GET_NEW_PASSWORD;
	    return;
	}
*/
	output_to_descriptor( d, (const char*) echo_on_str );
	output_to_descriptor( d, "\nWhat is your sex (M/F/N)? " );
	d->connected = CON_GET_NEW_SEX;
	break;

    case CON_GET_NEW_SEX:
	switch ( argument[0] )
	{
	case 'm': case 'M': ch->sex = SEX_MALE;    break;
	case 'f': case 'F': ch->sex = SEX_FEMALE;  break;
	case 'n': case 'N': ch->sex = SEX_NEUTRAL; break;
	default:
	    output_to_descriptor( d, "That's not a sex.\nWhat IS your sex? " );
	    return;
	}


	output_to_descriptor( d, "\nYou may choose from the following races, or type showstat [race] to learn more:\n" );
	buf[0] = '\0';
	buf2[0] = '\0';
	halfmax = (MAX_RACE/3) + 1;
	for ( iRace = 0; iRace < halfmax; iRace++ )
	{
	 if ( iRace == RACE_GOD )
	   continue;
         if (race_table[iRace].race_name[0] != '\0')
         {
          SPRINTF( buf2, "%-20s", race_table[iRace].race_name );
          STRAPP( buf, "%s", buf2 );
          SPRINTF( buf2, "%-20s", race_table[iRace+halfmax].race_name );
          STRAPP( buf, "%s", buf2 );
          if( iRace + (halfmax*2) < MAX_RACE )
          {
            SPRINTF( buf2, "%s", race_table[iRace+(halfmax*2)].race_name );
            STRAPP( buf, "%s", buf2 );
	  }
          STRAPP( buf, "\n" );
          output_to_descriptor( d, buf );
          buf[0] = '\0';
         }
        }
	STRAPP( buf, ": " );
	output_to_descriptor( d, buf );
	d->connected = CON_GET_NEW_RACE;
	break;

    case CON_GET_NEW_RACE:
	argument = one_argument(argument, arg);
        if (!str_cmp( arg, "help") )
        {
	      do_help(ch, argument);
      	      output_to_descriptor( d, "Please choose a race: ");
	      return;
	}
        if (!str_cmp( arg, "showstat") )
        {
	      do_showstatistic(ch, argument);
      	      output_to_descriptor( d, "Please choose a race: ");
	      return;
	}
	
	 for ( iRace = 0; iRace < MAX_RACE; iRace++ )
	 {
	    if ( toupper(arg[0]) == toupper(race_table[iRace].race_name[0])
	    &&   !str_prefix( arg, race_table[iRace].race_name ) )
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
    ||  race_table[iRace].race_name[0] == '\0')
	{
	    output_to_descriptor( d,
		"That's not a race.\nWhat IS your race? " );
	    return;
	}

	output_to_descriptor( d, "\nPlease choose a main ability from the folowing classes:\n" );
	buf[0] = '\0';
	buf2[0] = '\0';
	halfmax = (MAX_ABILITY/2) + 1;
	for ( iClass = 0; iClass < halfmax; iClass++ )
	{
	 if (ability_name[iClass] && ability_name[iClass][0] != '\0')
	 {
          SPRINTF( buf2, "%-20s", ability_name[iClass] );
          STRAPP( buf, "%s", buf2 );
          if( iClass + halfmax < MAX_ABILITY )
          {
            SPRINTF( buf2, "%s", ability_name[iClass+halfmax] );
            STRAPP( buf, "%s", buf2 );
	  }
          STRAPP( buf, "\n" );
          output_to_descriptor( d, buf );
          buf[0] = '\0';
         }
        }
	STRAPP( buf, ": " );
	output_to_descriptor( d, buf );
	d->connected = CON_GET_NEW_CLASS;
	break;

    case CON_GET_NEW_CLASS:
	argument = one_argument(argument, arg);
        if (!str_cmp( arg, "help") )
        {
	      do_help(ch, argument);
      	      output_to_descriptor( d, "Please choose an ability class: ");
	      return;
	}
	

	for ( iClass = 0; iClass < MAX_ABILITY; iClass++ )
	{
	    if ( toupper(arg[0]) == toupper(ability_name[iClass][0])
	    &&   !str_prefix( arg, ability_name[iClass] ) )
	    {
		ch->main_ability = iClass;
		break;
	    }
	}

    if ( iClass == MAX_ABILITY || iClass == 7 
    ||  !ability_name[iClass] || ability_name[iClass][0] == '\0')
	{
	    output_to_descriptor( d,
		"That's not a skill class.\nWhat IS it going to be? " );
	    return;
	}

	output_to_descriptor( d, "\nRolling stats....\n" );

    case CON_ROLL_STATS:

	    ch->perm_str = number_range(1, 6)+number_range(1, 6)+number_range(1, 6);
	    ch->perm_int = number_range(3, 6)+number_range(1, 6)+number_range(1, 6);
	    ch->perm_wis = number_range(3, 6)+number_range(1, 6)+number_range(1, 6);
	    ch->perm_dex = number_range(3, 6)+number_range(1, 6)+number_range(1, 6);
	    ch->perm_con = number_range(3, 6)+number_range(1, 6)+number_range(1, 6);
	    ch->perm_cha = number_range(3, 6)+number_range(1, 6)+number_range(1, 6);

	    ch->perm_str	 += race_table[ch->race].str_plus;
	    ch->perm_int	 += race_table[ch->race].int_plus;
	    ch->perm_wis	 += race_table[ch->race].wis_plus;
	    ch->perm_dex	 += race_table[ch->race].dex_plus;
	    ch->perm_con	 += race_table[ch->race].con_plus;
	    ch->perm_cha	 += race_table[ch->race].cha_plus;
	
	SPRINTF( buf, "\nSTR: %d  INT: %d  WIS: %d  DEX: %d  CON: %d  CHA: %d\n" ,
	    ch->perm_str, ch->perm_int, ch->perm_wis, 
	    ch->perm_dex, ch->perm_con, ch->perm_cha) ;
         
        output_to_descriptor( d, buf );
        output_to_descriptor( d, "\nAre these stats OK?. " );
	d->connected = CON_STATS_OK;
        break;
	
    case CON_STATS_OK:
		
	switch ( argument[0] )
	{
	case 'y': case 'Y': break;
	case 'n': case 'N': 
	    ch->perm_str = number_range(1, 6)+number_range(1, 6)+number_range(1, 6);
	    ch->perm_int = number_range(3, 6)+number_range(1, 6)+number_range(1, 6);
	    ch->perm_wis = number_range(3, 6)+number_range(1, 6)+number_range(1, 6);
	    ch->perm_dex = number_range(3, 6)+number_range(1, 6)+number_range(1, 6);
	    ch->perm_con = number_range(3, 6)+number_range(1, 6)+number_range(1, 6);
	    ch->perm_cha = number_range(3, 6)+number_range(1, 6)+number_range(1, 6);

	    ch->perm_str	 += race_table[ch->race].str_plus;
	    ch->perm_int	 += race_table[ch->race].int_plus;
	    ch->perm_wis	 += race_table[ch->race].wis_plus;
	    ch->perm_dex	 += race_table[ch->race].dex_plus;
	    ch->perm_con	 += race_table[ch->race].con_plus;
	    ch->perm_cha	 += race_table[ch->race].cha_plus;
	
	    SPRINTF( buf, "\nSTR: %d  INT: %d  WIS: %d  DEX: %d  CON: %d  CHA: %d\n" ,
	    ch->perm_str, ch->perm_int, ch->perm_wis, 
	    ch->perm_dex, ch->perm_con, ch->perm_cha) ;
         
            output_to_descriptor( d, buf );
            output_to_descriptor( d, "\nOK?. " );
	    return;
	default:
	    output_to_descriptor( d, "Invalid selection.\nYES or NO? " );
	    return;
	}

	output_to_descriptor( d, "\nWould you like ANSI or no graphic/color support, (R/A/N)? " );
	d->connected = CON_GET_WANT_RIPANSI;
        break;
        
    case CON_GET_WANT_RIPANSI:
	switch ( argument[0] )
	{
	case 'a': case 'A': SET_BIT(ch->act,PLR_ANSI);  break;
	case 'n': case 'N': break;
	default:
	    output_to_descriptor( d, "Invalid selection.\nANSI or NONE? " );
	    return;
	}
        output_to_descriptor( d, "Does your mud client have the Mud Sound Protocol? " );
	d->connected = CON_GET_MSP; 
	 break;
          

case CON_GET_MSP:
	switch ( argument[0] )
	{
	case 'y': case 'Y': SET_BIT(ch->act,PLR_SOUND);  break;
	case 'n': case 'N': break;
	default:
	    output_to_descriptor( d, "Invalid selection.\nYES or NO? " );
	    return;
	}
/*
	if ( !sysdata.WAIT_FOR_AUTH )
	{
*/	    SPRINTF( buf, "%s@%s new %s.", ch->name, d->host,
				race_table[ch->race].race_name);
	    log_string_plus( buf, LOG_COMM, sysdata.log_level);
	    to_channel( buf, CHANNEL_MONITOR, "Monitor", LEVEL_IMMORTAL );
	    output_to_descriptor( d, "Press [ENTER] " );
	    show_title(d);
	    {
	       int ability;
	       
	       for ( ability =0 ; ability < MAX_ABILITY ; ability++ )
	          ch->skill_level[ability] = 0;
	    }
	    ch->top_level = 0;
	    ch->position = POS_STANDING;
	    d->connected = CON_PRESS_ENTER;
	    return;
	    break;
/*	}

	output_to_descriptor( d, "\nYou now have to wait for a god to authorize you... please be patient...\n" );
	SPRINTF( buf, "(1) %s@%s new %s applying for authorization...",
				ch->name, d->host,
				race_table[ch->race].race_name);
	log_string( buf );
	to_channel( buf, CHANNEL_MONITOR, "Monitor", LEVEL_IMMORTAL );
	d->connected = CON_WAIT_1;
	break;

     case CON_WAIT_1:
	output_to_descriptor( d, "\nTwo more tries... please be patient...\n" );
	SPRINTF( buf, "(2) %s@%s new %s applying for authorization...",
				ch->name, d->host,
				race_table[ch->race].race_name);
	log_string( buf );
	to_channel( buf, CHANNEL_MONITOR, "Monitor", LEVEL_IMMORTAL );
	d->connected = CON_WAIT_2;
	break;

     case CON_WAIT_2:
	output_to_descriptor( d, "\nThis is your last try...\n" );
	SPRINTF( buf, "(3) %s@%s new %s applying for authorization...",
				ch->name, d->host,
				race_table[ch->race].race_name);
	log_string( buf );
	to_channel( buf, CHANNEL_MONITOR, "Monitor", LEVEL_IMMORTAL );
	d->connected = CON_WAIT_3;
	break;

    case CON_WAIT_3:
	output_to_descriptor( d, "Sorry... try again later.\n" );
	close_socket( d, FALSE );
	return;
	break;

    case CON_ACCEPTED:

	SPRINTF( buf, "%s@%s new %s.", ch->name, d->host,
				race_table[ch->race].race_name);
	log_string_plus( buf, LOG_COMM, sysdata.log_level );
	to_channel( buf, CHANNEL_MONITOR, "Monitor", LEVEL_IMMORTAL );
	output_to_descriptor( d, "\n" );
	show_title(d);
	    {
	       int ability;
	       
	       for ( ability =0 ; ability < MAX_ABILITY ; ability++ )
	          ch->skill_level[ability] = 0;
	    }
	ch->top_level = 0;
	ch->position = POS_STANDING;
	d->connected = CON_PRESS_ENTER;
	break;
*/
    case CON_PRESS_ENTER:
	if ( IS_SET(ch->act, PLR_ANSI) )
	  send_to_pager( "\033[2J", ch );
	else
	  send_to_pager( "\014", ch );
	if ( IS_IMMORTAL(ch) )
	{
	  send_to_pager( "&WImmortal Message of the Day&w\n", ch );
	  do_help( ch, "imotd" );
	}
	if ( ch->top_level > 0 )
	{
	  send_to_pager( "\n&WMessage of the Day&w\n", ch );
	  do_help( ch, "motd" );
	}
	if ( ch->top_level >= 100)
	{
	  send_to_pager( "\n&WAvatar Message of the Day&w\n", ch );
	  do_help( ch, "amotd" );
	}
	if ( ch->top_level == 0 )
	  do_help( ch, "nmotd" );
	send_to_pager( "\n&WPress [ENTER] &Y", ch );
        d->connected = CON_READ_MOTD;
        break;

    case CON_READ_MOTD:
	output_to_descriptor( d, "\nWelcome to Rise in Power...\n\n" );
	add_char( ch );
	d->connected	= CON_PLAYING;
        
        if ( !IS_NPC(ch) && IS_SET( ch->act , PLR_SOUND ) )
           send_to_char( "!!MUSIC(starwars.mid V=100)" , ch );
        
        
	if ( ch->top_level == 0 )
	{
	    OBJ_DATA *obj;
	    int iLang;
        
	    ch->pcdata->clan_name = STRALLOC( "" );
	    ch->pcdata->clan	  = NULL;
	    
	    ch->perm_lck = number_range(6, 20);
            ch->perm_frc = number_range(-800, 20);
	    ch->affected_by	  = race_table[ch->race].affected;
	    ch->perm_lck	 += race_table[ch->race].lck_plus;
	    ch->perm_frc	 += race_table[ch->race].frc_plus;
            
       if ( ch->main_ability == FORCE_ABILITY )
//         ch->perm_frc = URANGE( 1 , ch->perm_frc , 20 ); // Forcers always roll 25, per Sonja - DV 8/12/02
	   ch->perm_frc = 5;
	    else
	       ch->perm_frc = URANGE( 0 , ch->perm_frc , 20 );
	    /* Hunters do not recieve force */       
	    
	    if ( ch->main_ability == HUNTING_ABILITY )         
	      ch->perm_frc = 0;	    
	    
	    /* Droids do not recieve force */
	    
	    if( is_droid(ch) ) 
	      ch->perm_frc = 0;	    

	    /* Noghri are auto commando */
/*	    
	    if (ch->race == RACE_NOGHRI )
	    {
	       ch->pcdata->clan = get_clan( "The Death Commandos");
   	       ch->pcdata->clan_name = QUICKLINK( ch->pcdata->clan->name );
   	    }
*/	    
	    /* took out automaticly knowing common
	    
	    if ( (iLang = skill_lookup( "common" )) < 0 )
	    	bug( "Nanny: cannot find common language." );
	    else
	    	ch->pcdata->learned[iLang] = 100;
	    */
	    	
	    for ( iLang = 0; lang_array[iLang] != LANG_UNKNOWN; iLang++ )
	    	if ( lang_array[iLang] == race_table[ch->race].language )
	    		break;
	    if ( lang_array[iLang] == LANG_UNKNOWN )
	    	bug( "Nanny: invalid racial language." );
	    else
	    {
	    	if ( (iLang = skill_lookup( lang_names[iLang] )) < 0 )
	    		bug( "Nanny: cannot find racial language." );
	    	else
	    	{
	    		ch->pcdata->learned[iLang] = 100;
	    		ch->speaking	=  race_table[ch->race].language;
		   if ( ch->race == RACE_QUARREN &&
		   (iLang = skill_lookup( "quarren" )) >= 0 )
	    	   {
	    	        ch->pcdata->learned[iLang] = 100;
		        SET_BIT( ch->speaks , LANG_QUARREN );
		   }
		   if ( ch->race == RACE_MON_CALAMARI && 
		   (iLang = skill_lookup( "common" )) >= 0 )
	    	        ch->pcdata->learned[iLang] = 100;

		}
	    }

            ch->resistant           += race_table[ch->race].resist;
            ch->susceptible     += race_table[ch->race].suscept;

	    name_stamp_stats( ch );
            
	    {
	       int ability;
	       
	       for ( ability =0 ; ability < MAX_ABILITY ; ability++ )
	       {
	          ch->skill_level[ability] = 1;
	          ch->experience[ability] = 0;
	       }
	    }
	    ch->top_level = 1;
	    ch->hit	 = ch->max_hit;
            ch->hit     += race_table[ch->race].hit;
            ch->max_hit += race_table[ch->race].hit;
	    ch->move	 = ch->max_move;
	    ch->gold     = 5000;
	    if ( ch->perm_frc > 0 )
         	 ch->max_mana = 100 + 100*ch->perm_frc;
	    else
	         ch->max_mana = 0;
            ch->max_mana += race_table[ch->race].mana;
	    ch->mana	= ch->max_mana;
	    SPRINTF( buf, "%s the %s",ch->name,
		race_table[ch->race].race_name );
	    set_title( ch, buf );

            /* Added by Narn.  Start new characters with autoexit and autgold
               already turned on.  Very few people don't use those. */
            SET_BIT( ch->act, PLR_AUTOGOLD ); 
            SET_BIT( ch->act, PLR_AUTOEXIT ); 
            
            /* New players don't have to earn some eq */
            
            obj = create_object( get_obj_index(OBJ_VNUM_SCHOOL_BANNER), 0 );
	    obj_to_char( obj, ch );
	    equip_char( ch, obj, WEAR_LIGHT );
	    
	    /* armor they do though
	    obj = create_object( get_obj_index(OBJ_VNUM_SCHOOL_VEST), 0 );
	    obj_to_char( obj, ch );
	    equip_char( ch, obj, WEAR_BODY );

	    obj = create_object( get_obj_index(OBJ_VNUM_SCHOOL_SHIELD), 0 );
	    obj_to_char( obj, ch );
	    equip_char( ch, obj, WEAR_SHIELD );
            */
            
	    obj = create_object( get_obj_index(OBJ_VNUM_SCHOOL_DAGGER), 0 );
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
	        
	    if (!sysdata.WAIT_FOR_AUTH)
	    {
	      char_to_room( ch, get_room_index( ROOM_VNUM_SCHOOL ) );
	      ch->pcdata->auth_state = 3; 
	    }
	    else
	    {
	      char_to_room( ch, get_room_index( ROOM_VNUM_SCHOOL ) );
	      ch->pcdata->auth_state = 1;
	      SET_BIT(ch->pcdata->flags, PCFLAG_UNAUTHED);
	    }
	    /* Display_prompt interprets blank as default */
//	    ch->pcdata->prompt = STRALLOC("");
	}
	else
	if ( !IS_IMMORTAL(ch) && ch->pcdata->release_date > current_time )
	{
	    if ( ch->pcdata->jail_vnum )
	      char_to_room( ch, get_room_index(ch->pcdata->jail_vnum));
	    else
	      char_to_room( ch, get_room_index(6) );
	}
	else if ( ch->in_room && !IS_IMMORTAL( ch ) 
             && !IS_SET( ch->in_room->room_flags, ROOM_SPACECRAFT )
             && ch->in_room != get_room_index(6) )
	{
	    char_to_room( ch, ch->in_room );
	}
	else if ( ch->in_room && !IS_IMMORTAL( ch )  
             && IS_SET( ch->in_room->room_flags, ROOM_SPACECRAFT )
             && ch->in_room != get_room_index(6) )
	{
	    SHIP_DATA *ship;
	    
	    for ( ship = first_ship; ship; ship = ship->next )
	      if ( ch->in_room->vnum >= ship->firstroom && ch->in_room->vnum <= ship->lastroom )
                if ( ship->shipclass != SHIP_PLATFORM || ship->spaceobject ) 
                  char_to_room( ch, ch->in_room );
	}
	else
	{
	    char_to_room( ch, get_room_index( wherehome(ch) ) );
	}


    if ( IS_SET(ch->act, ACT_POLYMORPHED) )
	  REMOVE_BIT(ch->act, ACT_POLYMORPHED);
    if ( IS_SET(ch->act, PLR_QUESTOR) )
	  REMOVE_BIT(ch->act, PLR_QUESTOR);




    if ( get_timer( ch, TIMER_SHOVEDRAG ) > 0 )
        remove_timer( ch, TIMER_SHOVEDRAG );

    if ( get_timer( ch, TIMER_PKILLED ) > 0 )
	remove_timer( ch, TIMER_PKILLED );
    if ( ch->plr_home != NULL )
    	{
	 char filename[256];
         FILE *fph;
         ROOM_INDEX_DATA *storeroom = ch->plr_home;
         OBJ_DATA *obj;
         OBJ_DATA *obj_next;
           
         for ( obj = storeroom->first_content; obj; obj = obj_next )
	 {
	    obj_next = obj->next_content;
	    extract_obj( obj );
	 }

	 SPRINTF( filename, "%s%c/%s.home", PLAYER_DIR, tolower(ch->name[0]),
				 capitalize( ch->name ) );
	 if ( ( fph = fopen( filename, "r" ) ) != NULL )
	 {
//	    bool found;
	    OBJ_DATA *tobj, *tobj_next;

	    rset_supermob(storeroom);

//	    found = TRUE;
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
		if ( !str_cmp( word, "OBJECT" ) )	/* Objects	*/
		  fread_obj  ( supermob, fph, OS_CARRY );
		else
		if ( !str_cmp( word, "END"    ) )	/* Done		*/
		  break;
		else
		{
		    bug( "Load_plr_home: bad section.", 0 );
		    bug( ch->name, 0 );
		    break;
		}
	    }

	    FCLOSE( fph );

	    for ( tobj = supermob->first_carrying; tobj; tobj = tobj_next )
	    {
		tobj_next = tobj->next_content;
		obj_from_char( tobj );
                if( tobj->item_type != ITEM_MONEY )
		  obj_to_room( tobj, storeroom );
	    }
	    
	    release_supermob();

         }
    }
    

    if ( ch->pcdata->pet )
    {
           act( AT_ACTION, "$n returns with $s master.",
                      ch->pcdata->pet, NULL, ch, TO_NOTVICT );
           act( AT_ACTION, "$N returns with you.",
                        ch, NULL, ch->pcdata->pet, TO_CHAR );
    }         

    ch->pcdata->logon			= current_time;

    act( AT_ACTION, "$n has entered the game.", ch, NULL, NULL, TO_ROOM );
    do_look( ch, "auto" );
    mail_count(ch);
    break;

        /* Far too many possible screwups if we do it this way. -- Altrag */
/*        case CON_NEW_LANGUAGE:
        for ( iLang = 0; lang_array[iLang] != LANG_UNKNOWN; iLang++ )
		if ( !str_prefix( argument, lang_names[iLang] ) )
			if ( can_learn_lang( ch, lang_array[iLang] ) )
			{
				add_char( ch );
				SET_BIT( ch->speaks, lang_array[iLang] );
				set_char_color( AT_SAY, ch );
				ch_printf( ch, "You can now speak %s.\n", lang_names[iLang] );
				d->connected = CON_PLAYING;
				return;
			}
	set_char_color( AT_SAY, ch );
	output_to_descriptor( d, "You may not learn that language.  Please choose another.\n"
				  "New language: " );
	break;*/
    }

    return;
}



/*
 * Parse a name for acceptability.
 */
bool check_parse_name( char *name )
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
	char *pc;
	bool fIll;

	fIll = TRUE;
	for ( pc = name; *pc != '\0'; pc++ )
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
short check_reconnect( DESCRIPTOR_DATA *d, char *name, bool fConn )
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
		DISPOSE( d->character->pcdata->pwd );
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
		log_printf_plus( LOG_COMM, UMAX( sysdata.log_level, ch->top_level ), "%s@%s(%s) reconnected.", ch->name, d->host, d->user );

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
 
bool check_multi( DESCRIPTOR_DATA *d , char *name )
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
		log_printf_plus( LOG_COMM, sysdata.log_level, "%s attempting to multiplay with %s.", dold->original ? dold->original->name : dold->character->name , d->character->name );

	        d->character = NULL;
	        free_char( d->character );
	        return TRUE;
	}
    }

    return FALSE;

}                

short check_playing( DESCRIPTOR_DATA *d, char *name, bool kick )
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
        log_printf_plus( LOG_COMM, sysdata.log_level, "%s already connected.", ch->name );
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
	    log_printf_plus( LOG_COMM, UMAX( sysdata.log_level, ch->top_level ), "%s@%s reconnected, kicking off old link.",
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



/*
 * Write to one char. Commented out in favour of colour
 *
void send_to_char( const char *txt, CHAR_DATA *ch )
{
    if ( !ch )
    {
      bug( "Send_to_char: NULL *ch" );
      return;
    }
    if ( txt && ch->desc )
	output_to_descriptor( ch->desc, txt );
    return;
}
*/

/*
 * Same as above, but converts &color codes to ANSI sequences..
 */
void send_to_char_color( const char *txt, CHAR_DATA *ch )
{
    DESCRIPTOR_DATA *d;
    const char *colstr;
    const char *prevstr;
    char colbuf[20];
    int ln;

    if ( !ch )
    {
        bug( "Send_to_char_color: NULL *ch" );
        return;
    }

    if ( !txt || !(d = ch->desc) )
        return;

    prevstr = txt;

    /*
     * ============================================
     * STEP 1: Build fully colorized string FIRST
     * ============================================
     */
    size_t outsize = strlen(txt) * 8 + 32;
    char *out;
    CREATE(out, char, outsize);
    int outpos = 0;

    while ( (colstr = strpbrk(prevstr, "&^")) != NULL )
    {
        /* copy text before color code */
        int seglen = colstr - prevstr;
        if (seglen > 0)
        {
            memcpy(out + outpos, prevstr, seglen);
            outpos += seglen;
        }

        /* convert color code */
        ln = make_color_sequence(colstr, colbuf, d);

        if ( ln < 0 )
        {
            prevstr = colstr + 1;
            continue;
        }

        if ( ln > 0 )
        {
            memcpy(out + outpos, colbuf, ln);
            outpos += ln;
        }

        if (*(colstr + 1) != '\0')
            prevstr = colstr + 2;
        else
            prevstr = colstr + 1;
    }

    /* copy remaining text */
    if ( *prevstr )
    {
        int len = strlen(prevstr);
        memcpy(out + outpos, prevstr, len);
        outpos += len;
    }

    out[outpos] = '\0';

    output_to_descriptor(d, out);

    DISPOSE(out);
}

void write_to_pager( DESCRIPTOR_DATA *d, const char *txt, int length )
{
  if ( length <= 0 )
    length = strlen(txt);
  if ( length == 0 )
    return;
  if ( !d->pagebuf )
  {
    d->pagesize = MAX_STRING_LENGTH;
    CREATE( d->pagebuf, char, d->pagesize );
    d->pagelen = 0;
  }
  if ( d->pagepoint == PAGEPOINT_NULL)
  {
    d->pagepoint = 0;
    d->pagelen = 0;
    d->pagecmd = '\0';
  }
  if ( d->pagelen == 0 && !d->fcommand )
  {
    d->pagebuf[0] = '\n';
    d->pagebuf[1] = '\r';
    d->pagelen = 2;
  }
  while ( d->pagelen + (size_t)length + 1 >= d->pagesize )
  {
    if ( d->pagesize > 32000 )
    {
      bug( "Pager overflow.  Ignoring.\n" );
      d->pagelen = 0;
      d->pagepoint = PAGEPOINT_NULL;
      DISPOSE(d->pagebuf);
      d->pagesize = MAX_STRING_LENGTH;
      return;
    }
    d->pagesize *= 2;
    RECREATE(d->pagebuf, char, d->pagesize);
  }
    memcpy(d->pagebuf + d->pagelen, txt, length);  

    d->pagelen += length;                          
    d->pagebuf[d->pagelen] = '\0';   
  return;
}

/* commented out in favour of colour routine

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
    if ( IS_NPC(ch) || !IS_SET(ch->pcdata->flags, PCFLAG_PAGERON) )
    {
	send_to_char(txt, d->character);
	return;
    }
    write_to_pager(d, txt, 0);
  }
  return;
}

*/

void send_to_pager_color( const char *txt, CHAR_DATA *ch )
{
  DESCRIPTOR_DATA *d;
  const char *colstr;
  const char *prevstr = txt;
  char colbuf[MAX_INPUT_LENGTH];
  int ln;
  
  if ( !ch )
  {
    bug( "Send_to_pager_color: NULL *ch" );
    return;
  }
  if ( !txt || !ch->desc )
    return;

  d = ch->desc;
  ch = d->original ? d->original : d->character;

  if ( IS_NPC(ch) || !IS_SET(ch->pcdata->flags, PCFLAG_PAGERON) )
  {
    send_to_char_color(txt, d->character);
    return;
  }

    prevstr = txt;

    while ( (colstr = strpbrk(prevstr, "&^")) != NULL )
    {
        if ( colstr > prevstr )
            write_to_pager(d, prevstr, (colstr - prevstr));

        ln = make_color_sequence(colstr, colbuf, d);

        if ( ln < 0 )
        {
            prevstr = colstr + 1;
            break;
        }
        else if ( ln > 0 )
        {
            write_to_pager(d, colbuf, ln);
        }

        prevstr = colstr + 2;
    }

    if ( *prevstr )
        write_to_pager(d, prevstr, 0);

    return;
}


void set_char_color( sh_int AType, CHAR_DATA *ch )
{
    char buf[16];
    CHAR_DATA *och;
    
    if ( !ch || !ch->desc )
      return;
    
    och = (ch->desc->original ? ch->desc->original : ch);
    if ( !IS_NPC(och) && IS_SET(och->act, PLR_ANSI) )
    {
	if ( AType == 7 )
	  SPRINTF( buf, "\033[m" );
	else
	  SPRINTF(buf, "\033[0;%d;%s%dm", (AType & 8) == 8,
	        (AType > 15 ? "5;" : ""), (AType & 7)+30);
	output_to_descriptor( ch->desc, buf );
    }
    return;
}

void set_pager_color( sh_int AType, CHAR_DATA *ch )
{
    char buf[16];
    CHAR_DATA *och;
    
    if ( !ch || !ch->desc )
      return;
    
    och = (ch->desc->original ? ch->desc->original : ch);
    if ( !IS_NPC(och) && IS_SET(och->act, PLR_ANSI) )
    {
	if ( AType == 7 )
	  SPRINTF( buf, "\033[m" );
	else
	  SPRINTF(buf, "\033[0;%d;%s%dm", (AType & 8) == 8,
	        (AType > 15 ? "5;" : ""), (AType & 7)+30);
	send_to_pager( buf, ch );
	ch->desc->pagecolor = AType;
    }
    return;
}


/* source: EOD, by John Booth <???> */
void ch_printf(CHAR_DATA *ch, const char *fmt, ...)
{
    char buf[MAX_STRING_LENGTH * 2];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    send_to_char(buf, ch);
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

/*
 * The primary output interface for formatted output.
 */
/* Major overhaul. -- Alty */
#define NAME(ch)	(IS_NPC(ch) ? ch->short_descr : ch->name)

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
 *   pointer to static buffer containing formatted message
 *
 * Safety improvements:
 *
 *   • Prevents buffer overflow
 *   • Protects against NULL pointers
 *   • Handles missing arg1/arg2 safely
 *   • Limits writes to MAX_STRING_LENGTH
 */

static const char *he_she[]  = { "it",  "he",  "she" };
static const char *him_her[] = { "it",  "him", "her" };
static const char *his_her[] = { "its", "his", "her" };

char *act_string(const char *format, CHAR_DATA *to, CHAR_DATA *ch,
                 const void *arg1, const void *arg2)
{
    /* Output buffer for the final formatted string */
    thread_local static char buf[MAX_STRING_LENGTH];

    /* Temporary buffer used by some tokens (ex: $d) */
    char fname[MAX_INPUT_LENGTH];

    /* Write pointer into buf */
    char *point = buf;

    /*
     * Pointer to the last safe write position.
     * We reserve 3 bytes for:
     *
     *   "\n\0"
     */
    char *end = buf + sizeof(buf) - 3;

    /* Pointer to current position in format string */
    const char *str = format;

    /* Pointer to the string we will append */
    const char *i = NULL;

    /*
     * Interpret arg1 and arg2 in common ways.
     *
     * arg1 -> often object
     * arg2 -> often character
     */
    CHAR_DATA *vch = (CHAR_DATA *)arg2;
    OBJ_DATA *obj1 = (OBJ_DATA *)arg1;
    OBJ_DATA *obj2 = (OBJ_DATA *)arg2;

    /* Defensive checks */
    if (!format || !ch)
        return "";

    /*
     * Walk through the format string one character at a time
     */
    while (*str && point < end)
    {
        //If this is not a '$' token, just copy it directly
        if (*str != '$')
        {
            *point++ = *str++;
            continue;
        }

         //Skip the '$'
        str++;

         //If format string ended after '$', stop safely
        if (!*str)
            break;

         /* If a token requires arg2 but it was not provided,
         *  log a bug and insert a placeholder. */
        if (!arg2 && *str >= 'A' && *str <= 'Z')
        {
            bug("act_string: missing arg2 for code %c", *str);
            i = "<@@@>";
        }
        else
        {
            /*
             * Interpret token following '$'
             */
            switch (*str)
            {
            default:
                bug("act_string: bad code %c", *str);
                i = "<@@@>";
                break;

             //$t = string from arg1
            case 't':
                i = arg1 ? (char *)arg1 : "";
                break;

            // $T = string from arg2
            case 'T':
                i = arg2 ? (char *)arg2 : "";
                break;

            // $n = actor name (seen by 'to')
            case 'n':
                i = (to ? PERS(ch,to) : NAME(ch));
                break;

            // $N = victim name
            case 'N':
                i = vch ? (to ? PERS(vch,to) : NAME(vch)) : "someone";
                break;

            // $e / $E = he/she/it - CHAR
            case 'e':
                i = he_she[URANGE(0, ch->sex, 2)];
                break;
            // $e / $E = he/she/it - VICT
            case 'E':
                i = vch ? he_she[URANGE(0, vch->sex, 2)] : "it";
                break;

            // $m / $M = him/her/it - CHAR
            case 'm':
                i = him_her[URANGE(0, ch->sex, 2)];
                break;
            // $m / $M = him/her/it - VICT
            case 'M':
                i = vch ? him_her[URANGE(0, vch->sex, 2)] : "it";
                break;

            // $s / $S = his/her/its - CHAR
            case 's':
                i = his_her[URANGE(0, ch->sex, 2)];
                break;
            // $s / $S = his/her/its - VICT
            case 'S':
                i = vch ? his_her[URANGE(0, vch->sex, 2)] : "its";
                break;

            /*
             * $q = plural suffix
             * Used for messages like:
             *   "$n drop$q $p."
             * Which becomes:
             *   "Bob drops sword."
             *   "You drop sword."
             */
            case 'q':
                i = (to == ch) ? "" : "s";
                break;

            // $Q = possessive pronoun
            case 'Q':
                i = (to == ch) ? "your"
                               : his_her[URANGE(0, ch->sex, 2)];
                break;

            // $p = object short description
            case 'p':
                i = (!obj1 ? "something" :
                     (!to || can_see_obj(to,obj1)
                      ? obj_short(obj1) : "something"));
                break;

            // $P = second object
            case 'P':
                i = (!obj2 ? "something" :
                     (!to || can_see_obj(to,obj2)
                      ? obj_short(obj2) : "something"));
                break;

            /*
             * $d = door direction
             * Extracts first word from arg2.
             */
            case 'd':
                if (!arg2 || ((char *)arg2)[0] == '\0')
                    i = "door";
                else
                {
                    one_argument((char *)arg2, fname);
                    i = fname;
                }
                break;

            // $l = language string
            case 'l':
                i = lang_string(ch, to);
                break;
            }
        }

        /* Move past token */
        str++;

        if (!i)
            i = "";

        /*
         * Safely append i into the output buffer.
         * This prevents writing past MAX_STRING_LENGTH.
         */
        while (*i && point < end)
            *point++ = *i++;
    }

    // Terminate message with newline/carriage return
    *point++ = '\n';
    *point++ = '\r';
    *point   = '\0';

    // Capitalize the first character
    if (buf[0])
        buf[0] = UPPER(buf[0]);

    return buf;
}
#undef NAME
  
/*
 * act() - Cleaned up Act - AI/DV 3-15-26
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
void act( sh_int AType, const char *format, CHAR_DATA *ch,
          const void *arg1, const void *arg2, int type )
{
    char *txt;                     /* Final expanded message string */
    CHAR_DATA *to;                 /* Character receiving the message */
    CHAR_DATA *vch = (CHAR_DATA *)arg2; /* Victim character (if applicable) */

    /*
     * Ignore null or empty messages.
     */
    if ( !format || format[0] == '\0' )
        return;

    /*
     * Acting character must exist.
     */
    if ( !ch )
    {
        bug( "Act: null ch. (%s)", format );
        return;
    }

    /*
     * Determine who the initial recipient should be.
     *
     * TO_CHAR  -> message only to the acting character
     * otherwise -> start iterating through room occupants
     */
    if ( !ch->in_room )
        to = NULL;
    else if ( type == TO_CHAR )
        to = ch;
    else
        to = ch->in_room->first_person;

    /*
     * ACT_SECRETIVE mobs do not broadcast actions to the room.
     * Only the mob itself should see the message.
     */
    if ( IS_NPC(ch) && IS_SET(ch->act, ACT_SECRETIVE) && type != TO_CHAR )
        return;

    /*
     * Handle victim-targeted messages.
     *
     * Example:
     *   "$n hits you!"  (TO_VICT)
     */
    if ( type == TO_VICT )
    {
        if ( !vch )
        {
            bug( "Act: null vch with TO_VICT." );
            bug( "%s (%s)", ch->name, format );
            return;
        }

        if ( !vch->in_room )
        {
            bug( "Act: vch in NULL room!" );
            bug( "%s -> %s (%s)", ch->name, vch->name, format );
            return;
        }

        /* Send message directly to the victim */
        to = vch;
    }

    /*
     * Trigger room and object ACT_PROG scripts if applicable.
     *
     * These triggers allow rooms and objects to react to
     * player actions described by act() messages.
     */
    if ( MOBtrigger && type != TO_CHAR && type != TO_VICT && to )
    {
        OBJ_DATA *to_obj;

        /* Generate the message text without ANSI colors */
        txt = act_string(format, NULL, ch, arg1, arg2);

        /*
         * Room ACT trigger
         */
        if ( IS_SET(to->in_room->progtypes, ACT_PROG) )
            rprog_act_trigger(txt, to->in_room, ch,
                              (OBJ_DATA *)arg1, (void *)arg2);

        /*
         * Object ACT triggers for all objects in the room
         */
        for ( to_obj = to->in_room->first_content;
              to_obj;
              to_obj = to_obj->next_content )
        {
            if ( IS_SET(to_obj->pIndexData->progtypes, ACT_PROG) )
                oprog_act_trigger(txt, to_obj, ch,
                                  (OBJ_DATA *)arg1, (void *)arg2);
        }
    }

    /*
     * Send the message to appropriate characters.
     *
     * If TO_CHAR or TO_VICT, the loop runs once.
     * Otherwise we iterate through everyone in the room.
     */
    for ( ; to; to = (type == TO_CHAR || type == TO_VICT)
                     ? NULL : to->next_in_room )
    {

        if (!to)
            continue;      
        /*
         * Skip characters that should not receive the message.
         *
         * Conditions:
         *  - no descriptor (player not connected)
         *  - sleeping
         *  - NPC without relevant programs
         */
        if (((!to || !to->desc)
             && (IS_NPC(to) && !IS_SET(to->pIndexData->progtypes, ACT_PROG)))
            || !IS_AWAKE(to))
            continue;

        /*
         * Skip characters who cannot see the acting character.
         */
        if (!can_see(to, ch) && type != TO_VICT)
            continue;

        /*
         * Filter recipients based on target type.
         */
        if ( type == TO_CHAR && to != ch )
            continue;

        if ( type == TO_VICT && ( to != vch || to == ch ) )
            continue;

        if ( type == TO_ROOM && to == ch )
            continue;

        if ( type == TO_NOTVICT && (to == ch || to == vch) )
            continue;

        /*
         * Expand the format string into the final message.
         */
        txt = act_string(format, to, ch, arg1, arg2);

        /*
         * Send message to player with color.
         */
        if (to && to->desc)
        {
            set_char_color(AType, to);
            send_to_char_color(txt, to);
        }

        /*
         * Mob ACT trigger.
         *
         * Allows mobs to respond to actions happening nearby.
         */
        if (MOBtrigger)
        {
            mprog_act_trigger(txt, to, ch,
                              (OBJ_DATA *)arg1, (void *)arg2);
        }
    }

    /*
     * Ensure mob triggers remain enabled.
     */
    MOBtrigger = TRUE;
}

void do_name( CHAR_DATA *ch, char *argument )
{
  char fname[1024];
  struct stat fst;
  CHAR_DATA *tmp;
  char buf[MAX_STRING_LENGTH];

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
                        capitalize( argument ) );
  if ( stat( fname, &fst ) != -1 )
  {
    send_to_char("That name is already taken.  Please choose another.\n", ch);
    return;
  }

  STRFREE( ch->name );
  ch->name = STRALLOC( argument );
  SPRINTF( buf, "%s the %s",ch->name,
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

void display_prompt( DESCRIPTOR_DATA *d )
{
  CHAR_DATA *ch = d->character;
  CHAR_DATA *och = (d->original ? d->original : d->character);
  bool ansi = (!IS_NPC(och) && IS_SET(och->act, PLR_ANSI));
  const char *prompt;
  char buf[MAX_STRING_LENGTH];
  char *pbuf = buf;
  int stat;

  if ( !ch )
  {
    bug( "display_prompt: NULL ch" );
    return;
  }

  if ( !IS_NPC(ch) && ch->substate != SUB_NONE && ch->pcdata->subprompt
  &&   ch->pcdata->subprompt[0] != '\0' )
    prompt = ch->pcdata->subprompt;
  else
  if ( IS_NPC(ch) || !ch->pcdata->prompt || !*ch->pcdata->prompt )
    prompt = default_prompt(ch);
  else
    prompt = ch->pcdata->prompt;

  if ( ansi )
  {
    memmove(pbuf, "\033[m", 3);
    d->prevcolor = 0x07;
    pbuf += 3;
  }
  /* Clear out old color stuff */
/*  make_color_sequence(NULL, NULL, NULL);*/
  for ( ; *prompt; prompt++ )
  {
    /*
     * '&' = foreground color/intensity bit
     * '^' = background color/blink bit
     * '%' = prompt commands
     * Note: foreground changes will revert background to 0 (black)
     */
    if ( *prompt != '&' && *prompt != '^' && *prompt != '%' )
    {
      *(pbuf++) = *prompt;
      continue;
    }
    ++prompt;
    if ( !*prompt )
      break;
    if ( *prompt == *(prompt-1) )
    {
      *(pbuf++) = *prompt;
      continue;
    }
    switch(*(prompt-1))
    {
    default:
      bug( "Display_prompt: bad command char '%c'.", *(prompt-1) );
      break;
    case '&':
    case '^':
      stat = make_color_sequence(&prompt[-1], pbuf, d);
      if ( stat < 0 )
        --prompt;
      else if ( stat > 0 )
        pbuf += stat;
      break;
    case '%':
      *pbuf = '\0';
      stat = 0x80000000;
      switch(*prompt)
      {
      case '%':
	*pbuf++ = '%';
	*pbuf = '\0';
	break;
      case 'a':
	if ( ch->top_level >= 10 )
	  stat = ch->alignment;
	else if ( IS_GOOD(ch) )
    memmove(pbuf, "good", sizeof("good"));
	else if ( IS_EVIL(ch) )
	  memmove(pbuf, "evil", sizeof("evil"));
	else
	  memmove(pbuf, "neutral", sizeof("neutral"));
	break;
      case 'h':
	stat = ch->hit;
	break;
      case 'H':
	stat = ch->max_hit;
	break;
      case 'm':
	if ( IS_IMMORTAL(ch) || ch->skill_level[FORCE_ABILITY] > 1 )
	  stat = ch->mana;
	else
	  stat = 0;
	break;
      case 'M':
	if ( IS_IMMORTAL(ch) || ch->skill_level[FORCE_ABILITY] > 1 )
	  stat = ch->max_mana;
	else
	  stat = 0;
	break;
      case 'p':
        if ( ch->position == POS_RESTING )
          memmove(pbuf, "resting", sizeof("resting"));
        else if ( ch->position == POS_SLEEPING )
          memmove(pbuf, "sleeping", sizeof("sleeping"));
        else if ( ch->position == POS_SITTING )
          memmove(pbuf, "sitting", sizeof("sitting"));
        break;
      case 'u':
        stat = num_descriptors;
        break;
      case 'U':
        stat = sysdata.maxplayers;
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
        if ( IS_IMMORTAL(och) )
          stat = ch->in_room->vnum;
        break;
      case 'R':
        if ( IS_SET(och->act, PLR_ROOMVNUM) )
            snprintf( pbuf, MAX_STRING_LENGTH, "<#%d> ", ch->in_room->vnum );
        break;
      case 'i':
        if ( (!IS_NPC(ch) && IS_SET(ch->act, PLR_WIZINVIS)) ||
              (IS_NPC(ch) && IS_SET(ch->act, ACT_MOBINVIS)) )
          snprintf( pbuf, MAX_STRING_LENGTH, "(Invis %d) ", ( IS_NPC( ch ) ? ch->mobinvis : ch->pcdata->wizinvis ) );
          else
          if ( IS_AFFECTED(ch, AFF_INVISIBLE) )
            snprintf( pbuf, MAX_STRING_LENGTH, "(Invis) ");
          break;
      case 'I':
          stat = (IS_NPC(ch) ? (IS_SET(ch->act, ACT_MOBINVIS) ? ch->mobinvis : 0)
              : (IS_SET(ch->act, PLR_WIZINVIS) ? ch->pcdata->wizinvis : 0));
          break;
      }
      if ( stat != (int) 0x80000000 )
        snprintf( pbuf, MAX_STRING_LENGTH, "%d", stat );
      pbuf += strlen(pbuf);
      break;
    }
  }
  *pbuf = '\0';

    for (char *p = buf; *p; p++)
        printf("%02X ", (unsigned char)*p);
    printf("\n");

  output_to_descriptor(d, buf);
  return;
}

int make_color_sequence(const char *col, char *buf, DESCRIPTOR_DATA *d)
{
  int ln;
  const char *ctype = col;
  unsigned char cl;
  CHAR_DATA *och;
  bool ansi;
  
  och = (d->original ? d->original : d->character);
  ansi = (!IS_NPC(och) && IS_SET(och->act, PLR_ANSI));
  col++;
  if ( !*col )
    ln = -1;
  else if ( *ctype != '&' && *ctype != '^' )
  {
    bug("Make_color_sequence: command '%c' not '&' or '^'.", *ctype);
    ln = -1;
  }
  else if ( *col == *ctype )
  {
    buf[0] = *col;
    buf[1] = '\0';
    ln = 1;
  }
  else if ( !ansi )
    ln = 0;
  else
  {
    cl = d->prevcolor;
    switch(*ctype)
    {
    default:
      bug( "Make_color_sequence: bad command char '%c'.", *ctype );
      ln = -1;
      break;
    case '&':
      if ( *col == '-' )
      {
        buf[0] = '~';
        buf[1] = '\0';
        ln = 1;
        break;
      }
    case '^':
      {
        int newcol;
        
        if ( (newcol = getcolor(*col)) < 0 )
        {
          ln = 0;
          break;
        }
        else if ( *ctype == '&' )
          cl = (cl & 0xF0) | newcol;
        else
          cl = (cl & 0x0F) | (newcol << 4);
      }
      if ( cl == d->prevcolor )
      {
        ln = 0;
        break;
      }
      memmove(buf, "\033[", sizeof("\033["));
      ln = sizeof("\033[") - 1;

      if ((cl & 0x88) != (d->prevcolor & 0x88))
      {
          ln += snprintf(buf + ln, MAX_STRING_LENGTH - ln, "m\033[");

          if (cl & 0x08)
              ln += snprintf(buf + ln, MAX_STRING_LENGTH - ln, "1;");

          if (cl & 0x80)
              ln += snprintf(buf + ln, MAX_STRING_LENGTH - ln, "5;");

          d->prevcolor = 0x07 | (cl & 0x88);
      }
      else
        ln = 2;
      if ((cl & 0x07) != (d->prevcolor & 0x07))
      {
          ln += snprintf(buf + ln, MAX_STRING_LENGTH - ln, "3%d;", cl & 0x07);
      }

      if ((cl & 0x70) != (d->prevcolor & 0x70))
      {
          ln += snprintf(buf + ln, MAX_STRING_LENGTH - ln, "4%d;", (cl & 0x70) >> 4);
      }
      if ( buf[ln-1] == ';' )
        buf[ln-1] = 'm';
      else
      {
        buf[ln++] = 'm';
        buf[ln] = '\0';
      }
      d->prevcolor = cl;
    }
  }
  if ( ln <= 0 )
    *buf = '\0';
  return ln;
}

void set_pager_input( DESCRIPTOR_DATA *d, char *argument )
{
  while ( isspace(*argument) )
    argument++;
  d->pagecmd = *argument;
  return;
}

bool pager_output( DESCRIPTOR_DATA *d )
{
    size_t last; 
    CHAR_DATA *ch;
    int pclines;
    int lines;
//  bool ret;

    if ( !d || d->pagepoint == PAGEPOINT_NULL || d->pagecmd == -1 )
        return TRUE;

    ch = d->original ? d->original : d->character;

    int term_lines = 0;

    if (d->term_height > 0)
        term_lines = d->term_height - 2;  // leave room for prompt
    else
        term_lines = 24;                 // sane fallback

    pclines = UMAX(ch->pcdata->pagerlen, 5);

    /* Use the smaller of user setting vs terminal size */
    if (term_lines > 0)
        pclines = UMIN(pclines, term_lines);

    pclines = UMAX(pclines - 1, 3);
    
    switch(LOWER(d->pagecmd))
    {
    default:
        lines = 0;
        break;
    case 'b':
        lines = -1-(pclines*2);
        break;
    case 'r':
        lines = -1-pclines;
        break;
    case 'q':
        d->pagelen = 0;
        d->pagepoint = PAGEPOINT_NULL;
        flush_buffer(d, TRUE);
        DISPOSE(d->pagebuf);
        d->pagesize = MAX_STRING_LENGTH;
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
    if ( d->pagepoint < d->pagelen &&
         d->pagebuf[d->pagepoint] == '\n' )
    {
        if ( d->pagepoint + 1 < d->pagelen &&
             d->pagebuf[d->pagepoint + 1] == '\r' )
        {
            d->pagepoint += 2;
        }
    }

    /* =========================
     * FORWARD SCAN
     * ========================= */
    for ( lines = 0, last = d->pagepoint; lines < pclines; ++last )
    {
        if ( last >= d->pagelen ) 
            break;
        else if ( d->pagebuf[last] == '\n' )
            ++lines;
    }
    
    if ( last < d->pagelen && d->pagebuf[last] == '\r' )
        ++last;

    /* =========================
     * OUTPUT CHUNK
     * ========================= */
    if ( last != d->pagepoint )
    {
        write_to_buffer(d, d->pagebuf + d->pagepoint, last - d->pagepoint);
        d->pagepoint = last;
    }

    /* =========================
     * SKIP WHITESPACE
     * ========================= */
    while ( last < d->pagelen &&
            (d->pagebuf[last] == ' ' || d->pagebuf[last] == '\t') )
        ++last;

    if ( last >= d->pagelen )
    {
        d->pagelen = 0;
        d->pagepoint = PAGEPOINT_NULL;
        flush_buffer(d, TRUE);
        DISPOSE(d->pagebuf);
        d->pagesize = MAX_STRING_LENGTH;
        return TRUE;
    }

    d->pagecmd = -1;
    if ( IS_SET( ch->act, PLR_ANSI ) )
        output_to_descriptor(d, "\033[1;36m");
    output_to_descriptor(d,"(C)ontinue, (R)efresh, (B)ack, (Q)uit: [C] ");
    if ( IS_SET( ch->act, PLR_ANSI ) )
    {
        char buf[32];

        if ( d->pagecolor == 7 )
        memmove( buf, "\033[m", sizeof("\033[m") );
        else
        SPRINTF(buf, "\033[0;%d;%s%dm", (d->pagecolor & 8) == 8,
            (d->pagecolor > 15 ? "5;" : ""), (d->pagecolor & 7)+30);
        output_to_descriptor( d, buf );
    }
    flush_buffer(d, FALSE);    
    return TRUE;
}

void whocount(struct tm tmdata)
{
  char datebuf[32];
  char timebuf[32];
  char countbuf[64];
  int player_count = 0;
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

void *zlib_alloc(void *opaque, unsigned int items, unsigned int size)
{
    return calloc(items, size);
}

void zlib_free(void *opaque, void *address)
{
    DISPOSE(address);
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
            nBlock = UMIN (len - iStart, 16384);
            if ((nWrite = write(d->descriptor, d->out_compress_buf + iStart, nBlock)) < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK || errno == ENOSR)
                    break;

                return FALSE;
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
    bug("MCCP START BYTES: sending IAC SB COMPRESS2 IAC SE");    

    CREATE(s, z_stream, 1);
    CREATE(d->out_compress_buf, unsigned char, COMPRESS_BUF_SIZE);

    s->next_in = NULL;
    s->avail_in = 0;

    s->next_out = d->out_compress_buf;
    s->avail_out = COMPRESS_BUF_SIZE;

    s->zalloc = zlib_alloc;
    s->zfree  = zlib_free;
    s->opaque = NULL;

    if (deflateInit(s, 9) != Z_OK)
    {
        DISPOSE(d->out_compress_buf);
        DISPOSE(s);
        return FALSE;
    }

    if (telopt == TELOPT_COMPRESS)
        output_to_descriptor(d, enable_compress);
    else if (telopt == TELOPT_COMPRESS2)
        output_to_descriptor(d, enable_compress2);
    else
        bug("compressStart: bad TELOPT passed");
    flush_buffer(d, FALSE);
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

    d->out_compress->avail_in = 0;
    d->out_compress->next_in = dummy;

    if (deflate(d->out_compress, Z_FINISH) == Z_STREAM_END)
      process_compressed(d); /* try to send any residual data */

    deflateEnd(d->out_compress);
    DISPOSE(d->out_compress_buf);
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

static char *format_divider_line(const char *src, int width)
{
    char fill = '-';

    /* detect fill character */
    for (const char *p = src; *p; p++)
    {
        if (*p == '-' || *p == '=' || *p == '_')
        {
            fill = *p;
            break;
        }
    }

    /* preserve leading color codes like &C */
    char prefix[32] = "";
    int pi = 0;

    const char *p = src;
    while (*p == '&' && *(p+1))
    {
        if (pi < (int)sizeof(prefix) - 2)
        {
            prefix[pi++] = *p++;
            prefix[pi++] = *p++;
        }
        else break;
    }
    prefix[pi] = '\0';

    int prefix_len = strlen(prefix);

    /* allocate output */
    int total = width + prefix_len + 3;
    char *out = (char *)malloc(total);

    int o = 0;

    /* copy prefix */
    for (int i = 0; i < prefix_len; i++)
        out[o++] = prefix[i];

    /* fill line */
    for (int i = 0; i < width; i++)
        out[o++] = fill;

    out[o++] = '\n';
    out[o] = '\0';

    return out;
}

char *wrap_text_ex(const char *txt, int width, int flags, int indent)
{
    if (!txt)
        return str_dup("");

    if (width <= 0 || (flags & WRAP_NO_WRAP))
        return str_dup(txt);

    int len = strlen(txt);
    int outsize = len * 4 + 1;
    char *out = (char *)malloc(outsize);

    int o = 0;
    int col = 0;

    int last_space_out = -1;
    int last_space_col = -1;
    int last_space_i = -1;    

    int line = 0;

    /* helper: apply indent */
    #define APPLY_INDENT() \
        if ((flags & WRAP_INDENT) || \
           ((flags & WRAP_HANGING_INDENT) && line > 0)) \
        { \
            for (int k = 0; k < indent; k++) \
            { \
                if (o >= outsize - 8) \
                { \
                    outsize *= 2; \
                    out = (char *)realloc(out, outsize); \
                } \
                out[o++] = ' '; \
                col++; \
            } \
        }
#define ENSURE_SPACE(n) \
    do { \
        if (o + (n) >= outsize) { \
            outsize *= 2; \
            void *tmp_void = realloc(out, outsize); \
            if (!tmp_void) { free(out); return str_dup(""); } \
            out = (char *)tmp_void; \
        } \
    } while(0)

    APPLY_INDENT();

    for (int i = 0; i < len; i++)  
    {
        /* ensure space */
        ENSURE_SPACE(2);

        /* =========================
         * ANSI handling
         * ========================= */
        if (!(flags & WRAP_NO_COLOR) && txt[i] == '\x1b')
        {
            ENSURE_SPACE(2);

            out[o++] = txt[i++];  /* copy ESC and advance */

            while (i < len)
            {
                ENSURE_SPACE(2);
                char c = txt[i];
                out[o++] = c;
                i++;              /* advance AFTER using */

                if (c == 'm')
                    break;
            }

            i--; /* compensate for for-loop increment */
            continue;
        }

        /* =========================
         * NEW: SMAUG color handling (&X)
         * ========================= */
        if (!(flags & WRAP_NO_COLOR) && txt[i] == '&' && i + 1 < len)
        {
            /* NEW: handle escaped && (prints literal &) */
            if (txt[i+1] == '&')
            {
                ENSURE_SPACE(2);

                out[o++] = '&';
                i++;        /* skip second & */
                col++;      /* visible char */
                continue;
            }

            ENSURE_SPACE(3);

            out[o++] = txt[i++]; /* '&' */
            out[o++] = txt[i];   /* color code */

            continue;
        }        

        /* =========================
         * newline handling
         * ========================= */
        if (txt[i] == '\n' || txt[i] == '\r')
        {
            if (txt[i] == '\n')
            {
                ENSURE_SPACE(2);        
                out[o++] = '\n';
                col = 0;

                last_space_out = -1;
                last_space_col = -1;
                last_space_i = -1;                

                line++;
                APPLY_INDENT();
            }

            continue;
        }

        /* =========================
        * TAB handling (expand to spaces)
        * ========================= */
        if (txt[i] == '\t')
        {
            int spaces = 4 - (col % 4);

            for (int s = 0; s < spaces; s++)
            {
                ENSURE_SPACE(1);
                out[o++] = ' ';
                col++;

                /* track spaces for wrapping */
                last_space_out = o - 1;
                last_space_col = col - 1;
                last_space_i = 1;                  
            }

            continue;
        }

        /* =========================
         * preserve lines mode
         * ========================= */
        if (flags & WRAP_PRESERVE_LINES)
        {
            ENSURE_SPACE(2);           
            out[o++] = txt[i];
            col++;
            continue;
        }

        /* =========================
         * wrap before overflow
         * ========================= */
        if (col + 1 > width)
        {
            if (last_space_out != -1)
            {
                if (last_space_i < 0 || last_space_i >= len)   /* 🔹 FIX */
                {
                    /* fallback to hard wrap */
                    out[o++] = '\n';
                    col = 0;
                    line++;
                    APPLY_INDENT();
                    continue;
                }                
                o = last_space_out;
                out[o++] = '\n';

                /* 🔹 FIX: jump to safe input position */
                i = last_space_i - 1;

                col = 0;
                line++;

                last_space_out = -1;
                last_space_col = -1;
                last_space_i = -1;

                APPLY_INDENT();
                continue;
            }
            else
            {
                /* hard wrap */
                out[o++] = '\n';
                col = 0;

                line++;
                APPLY_INDENT();
                continue;
            }
        }

        /* =========================
         * normal char
         * ========================= */
        out[o++] = txt[i];
        col++;

        /* track spaces */
        if (txt[i] == ' ')
        {
            last_space_out = o - 1;
            last_space_col = col - 1;
            last_space_i = i;
        }
    }

    out[o] = '\0';
    return out;
}

static inline bool is_smaug_color(char c)
{
    return isalnum((unsigned char)c);
}

size_t visible_length(const char *txt)
{
    if (!txt)
        return 0;

    size_t len = 0;

    for (int i = 0; txt[i] != '\0'; i++)
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

            continue;
        }
        /* =========================
         * SMAUG color codes (&x, ^x)
         * ========================= */
        if ((txt[i] == '&' || txt[i] == '^'))
        {
            if (txt[i + 1] == '\0')
                break;  // nothing to look ahead to

            if (is_smaug_color(txt[i + 1]))
            {
                i++;  // skip color code
                continue;
            }
        }

        /* =========================
         * Normal visible character
         * ========================= */
        len++;
    }

    return len;
}

size_t visible_length_range(const char *start, const char *end, int flags)
{
    size_t len = 0;
    const char *p = start;

    while (p < end && *p)
    {
        if (!(flags & WRAP_NO_COLOR) && *p == '\x1b')
        {
            p++;
            while (p < end && *p && *p != 'm')
                p++;
            if (p < end && *p)
                p++;
            continue;
        }

        if (!(flags & WRAP_NO_COLOR) && (*p == '&' || *p == '^'))
        {
            if ((p + 1) < end && is_smaug_color(*(p + 1)))
            {
                p += 2;
                continue;
            }
        }

        len++;
        p++;
    }

    return len;
}

size_t visible_length_ex(const char *txt, int flags)
{
    if (!txt)
        return 0;

    size_t len = 0;
    const char *p = txt;

    while (*p)
    {
        /* ANSI escape */
        if (!(flags & WRAP_NO_COLOR) && *p == '\x1b')
        {
            p++;

            if (!*p)
                break;

            while (*p && *p != 'm')
                p++;

            if (!*p)
                break;

            p++;  // skip 'm'
            continue;
        }

        /* SMAUG color */
        if (!(flags & WRAP_NO_COLOR) && (*p == '&' || *p == '^'))
        {
            if (!*(p + 1))
                break;

            if (is_smaug_color(*(p + 1)))
            {
                p += 2;
                continue;
            }
        }

        len++;
        p++;
    }

    return len;
}

bool looks_preformatted(const char *txt)
{
    if (!txt || !*txt)
        return FALSE;

    if (visible_length(txt) == 0)
        return TRUE;

    int lines = 0;
    int short_lines = 0;
    int long_lines = 0;
    int indented_lines = 0;

    while (*txt == '\n')
        txt++;

    const char *p = txt;
    const char *line_start = p;

    while (*p)
    {
        if (*p == '\n')
        {
            size_t len = visible_length_range(line_start, p, 0);

            /* Skip empty lines entirely */
            if (len == 0)
            {
                line_start = p + 1;
                p++;
                continue;
            }
            lines++;

            if (len < 60)
                short_lines++;

            if (len > 90)
                long_lines++;

            /* Detect indentation (spaces at start) */
            if (*line_start == ' ' || *line_start == '\t')
                indented_lines++;

            line_start = p + 1;
        }

        p++;
    }

    /* Handle last line (no trailing newline) */
    if (line_start != p)
    {
        size_t len = visible_length_range(line_start, p, 0);
        lines++;

        if (len < 60)
            short_lines++;

        if (len > 90)
            long_lines++;

        if (*line_start == ' ' || *line_start == '\t')
            indented_lines++;
    }

    /*
     * ============================================================
     * Heuristics
     * ============================================================
     */

    if (lines <= 1)
        return FALSE;

    /*
     * Strong signal: lots of short lines
     */
    if (lines > 3 && short_lines > lines / 2)
        return TRUE;

    /*
     * Strong signal: indentation present (helps, lists)
     */
    if (indented_lines > 0 && lines > 3)
        return TRUE;

    /*
     * Mixed lengths (manual formatting)
     */
    if (short_lines > 0 && long_lines > 0)
        return TRUE;

    return FALSE;
}

bool is_structured_line(const char *line)
{
    if (!line || !*line)
        return FALSE;

    /* Indentation */
    if (*line == ' ' || *line == '\t')
        return TRUE;

    /* Bullets */
    if (*line == '-' || *line == '*' || *line == '+')
        return TRUE;

    /* Table-ish */
    if (strchr(line, '|'))
        return TRUE;

    return FALSE;
}

char *wrap_text_smart(const char *txt, int width)
{
    if (!txt)
        return strdup("");


    if (visible_length(txt) == 0)
        return strdup(txt);

    size_t cap = strlen(txt) * 2 + 32;
    size_t len = 0;
    char *out = (char *)malloc(cap);
    out[0] = '\0';

    const char *p = txt;

while (*p)
    {
        /*
         * ============================================
         * STEP 1: Find paragraph boundary safely
         * Supports:
         *   "\n\n"
         *   "\r\n\n"
         * ============================================
         */
        const char *start = p;
        const char *end = NULL;

        const char *scan = p;
        while (*scan)
        {
            if (scan[0] == '\n' && scan[1] == '\n')
            {
                end = scan;
                break;
            }
            if (scan[0] == '\n' && scan[1] == '\r' &&
                scan[2] == '\n' && scan[3] == '\r')
            {
                end = scan;
                break;
            }
            scan++;
        }

        if (!end)
            end = p + strlen(p);

        size_t plen = (size_t)(end - start);

        /*
         * Skip empty paragraphs cleanly
         */
        if (plen == 0)
        {
            if (end[0] == '\n' && end[1] == '\n')
                p = end + 2;
            else
                p = end;  /* fallback safety */

            continue;
        }

        /*
         * ============================================
         * STEP 2: Copy paragraph into temp buffer
         * (Still string-based for compatibility)
         * ============================================
         */
        char *para = (char *)malloc(plen + 1);
        memcpy(para, start, plen);
        para[plen] = '\0';

        /*
         * ============================================
         * STEP 3: Decide formatting strategy
         * ============================================
         */
        char *processed = NULL;
        bool processed_owned = FALSE;

        if (is_divider_line(para))
        {
            processed = format_divider_line(para, width);
            processed_owned = TRUE;
        }
        else if (is_structured_line(para) || looks_preformatted(para))
        {
            /*
             * Pass through unchanged
             */
            processed = para;
            processed_owned = FALSE;
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
            processed = wrap_text_ex(para, width, WRAP_NONE, 0);

            if (processed != para)
            {
                processed_owned = TRUE;
            }
            else
            {
                processed_owned = FALSE;
            }
        }

        /*
         * ============================================
         * STEP 4: Append to output buffer
         * append_str() must:
         *   - grow buffer if needed
         *   - maintain null termination
         * ============================================
         */
        append_str(&out, &len, &cap, processed);

        /*
         * ============================================
         * STEP 5: Cleanup memory safely
         * ============================================
         */
        if (processed_owned)
            free(processed);

        free(para);

        /*
         * ============================================
         * STEP 6: Preserve paragraph spacing
         * ============================================
         */
        if (*end)
        {
            append_str(&out, &len, &cap, "\n\n");

            /* Advance pointer past delimiter */
            if (end[0] == '\r')
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

static void append_str(char **buf, size_t *len, size_t *cap, const char *src)
{
    if (!src)
        return;

    size_t slen = strlen(src);

    if (*len + slen + 1 > *cap)
    {
        size_t newcap = (*cap) * 2;
        while (*len + slen + 1 > newcap)
            newcap *= 2;

        char *tmp = (char *)realloc(*buf, newcap);
        if (!tmp)
        {
            bug("append_str: realloc failed");
            return;
        }

        *buf = tmp;
        *cap = newcap;
    }

    memcpy(*buf + *len, src, slen);
    *len += slen;
    (*buf)[*len] = '\0';
}

