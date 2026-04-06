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
*			   Wizard/god command module      		   *
****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "mud.h"

#define RESTORE_INTERVAL 21600

char * const save_flag[] =
{ "death", "kill", "passwd", "drop", "put", "give", "auto", "zap",
"auction", "get", "receive", "idle", "backup", "r13", "r14", "r15", "r16",
"r17", "r18", "r19", "r20", "r21", "r22", "r23", "r24", "r25", "r26", "r27",
"r28", "r29", "r30", "r31" };



/*
 * Local functions.
 */
ROOM_INDEX_DATA * find_location	args( ( CHAR_DATA *ch, char *arg ) );
void              save_banlist  args( ( void ) );
void              close_area    args( ( AREA_DATA *pArea ) );
int               get_color (char *argument); /* function proto */
bool 		  check_for_immroom( CHAR_DATA *ch, ROOM_INDEX_DATA *location);


/*
 * Global variables.
 */

char reboot_time[50];
time_t new_boot_time_t;
extern struct tm new_boot_struct;
extern OBJ_INDEX_DATA *obj_index_hash[MAX_KEY_HASH];
extern MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];

bool check_for_immroom( CHAR_DATA *ch, ROOM_INDEX_DATA *location)
{
    if (ch->top_level == LEVEL_SUPREME ? 0: (location->vnum == IMP_ROOM1?1:(location->vnum == IMP_ROOM2?1:0)))
      return TRUE;
    return FALSE;
}

int get_saveflag( char *name )
{
    unsigned int x;

    for ( x = 0; x < sizeof(save_flag) / sizeof(save_flag[0]); x++ )
      if ( !str_cmp( name, save_flag[x] ) )
        return x;
    return -1;
}

void do_wizhelp( CHAR_DATA *ch, char *argument )
{
    CMDTYPE * cmd;
    int col, hash;

    col = 0;
    set_pager_color( AT_PLAIN, ch );
    for ( hash = 0; hash < 126; hash++ )
	for ( cmd = command_hash[hash]; cmd; cmd = cmd->next )
	    if ( cmd->level >= LEVEL_HERO
	    &&   cmd->level <= get_trust( ch ) 
            && (!cmd->commandgroup.any() || (cmd->commandgroup.intersects(ch->pcdata->commandgroup))))
	    {
		pager_printf( ch, "%-12s", cmd->name );
		if ( ++col % 6 == 0 )
		    send_to_pager( "\n", ch );
	    }

    if ( col % 6 != 0 )
	send_to_pager( "\n", ch );
    return;
}

void do_restrict( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    sh_int level, hash;
    CMDTYPE *cmd;
    bool found;

    found = FALSE;

    argument = one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
	send_to_char( "Restrict which command?\n", ch );
	return;
    }

    argument = one_argument ( argument, arg2 );
    if ( arg2[0] == '\0' )
      level = get_trust( ch );
    else
      level = atoi( arg2 );

    level = UMAX( UMIN( get_trust( ch ), level ), 0 );

    hash = arg[0] % 126;
    for ( cmd = command_hash[hash]; cmd; cmd = cmd->next )
    {
	if ( !str_prefix( arg, cmd->name )
	&&    cmd->level <= get_trust( ch ) )
	{
	    found = TRUE;
	    break;
	}
    }

    if ( found )
    {
    	if ( !str_prefix( arg2, "show" ) )
    	{
    		SPRINTF(buf, "%s show", cmd->name);
    		do_cedit(ch, buf);
/*    		ch_printf( ch, "%s is at level %d.\n", cmd->name, cmd->level );*/
    		return;
    	}
	cmd->level = level;
	ch_printf( ch, "You restrict %s to level %d\n",
	   cmd->name, level );
	log_printf( buf, "%s restricting %s to level %d",
	     ch->name, cmd->name, level );
    }
    else
    	send_to_char( "You may not restrict that command.\n", ch );

    return;
}

/* 
 * Check if the name prefix uniquely identifies a char descriptor
 */ 
CHAR_DATA *get_waiting_desc( CHAR_DATA *ch, char *name ) 
{ 
  DESCRIPTOR_DATA *d; 
  CHAR_DATA       *ret_char; 
  static unsigned int number_of_hits; 
  
  number_of_hits = 0; 
  for ( d = first_descriptor; d; d = d->next ) 
    { 
    if ( d->character && (!str_prefix( name, d->character->name )) &&
         IS_WAITING_FOR_AUTH(d->character) )
      { 
      if ( ++number_of_hits > 1 ) 
         { 
         ch_printf( ch, "%s does not uniquely identify a char.\n", name ); 
         return NULL; 
         } 
      ret_char = d->character;       /* return current char on exit */
      } 
    } 
  if ( number_of_hits==1 ) 
     return ret_char; 
  else 
     { 
     send_to_char( "No one like that waiting for authorization.\n", ch ); 
     return NULL; 
     } 
} 

void do_authorize( CHAR_DATA *ch, char *argument )
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  CHAR_DATA *victim;
  DESCRIPTOR_DATA *d;

  argument = one_argument( argument, arg1 );
  argument = one_argument( argument, arg2 );

  if ( arg1[0] == '\0' )
     {
     send_to_char( "Usage:  authorize <player> <yes|name|no/deny>\n", ch );
     send_to_char( "Pending authorizations:\n", ch );
     send_to_char( " Chosen Character Name\n", ch );
     send_to_char( "---------------------------------------------\n", ch );
     for ( d = first_descriptor; d; d = d->next )
         if ( (victim = d->character) != NULL && IS_WAITING_FOR_AUTH(victim) )
	    ch_printf( ch, " %s@%s new %s...\n",
               victim->name,
               victim->desc->host,
               race_table[victim->race].race_name );
    return;
    }

  victim = get_waiting_desc( ch, arg1 );
  if ( victim == NULL )
     return;

  if ( arg2[0]=='\0' || !str_cmp( arg2,"accept" ) || !str_cmp( arg2,"yes" ))
     {
	victim->pcdata->auth_state = 3;
	BV_REMOVE_BIT(victim->pcdata->flags, PCFLAG_UNAUTHED);
	if ( victim->pcdata->authed_by )
	   STRFREE( victim->pcdata->authed_by );
	victim->pcdata->authed_by = QUICKLINK( ch->name );
	SPRINTF( buf, "%s authorized %s", ch->name,
					  victim->name );
	to_channel( buf, CHANNEL_MONITOR, "Monitor", ch->top_level );
	ch_printf( ch, "You have authorized %s.\n", victim->name);
    
     /* Below sends a message to player when name is accepted - Brittany   */                           
        
        ch_printf( victim,                                            /* B */
        "The MUD Administrators have accepted the name %s.\n"       /* B */
        "You are now fully authorized to play Rise in Power.\n",victim->name);                               /* B */
     return;
     }
     else if ( !str_cmp( arg2, "no" ) || !str_cmp( arg2, "deny" ) )
     {
	send_to_char( "You have been denied access.\n", victim);
	SPRINTF( buf, "%s denied authorization to %s", ch->name,
						       victim->name );
	to_channel( buf, CHANNEL_MONITOR, "Monitor", ch->top_level );
	ch_printf( ch, "You have denied %s.\n", victim->name);
	do_quit(victim, "");
     }

     else if ( !str_cmp( arg2, "name" ) || !str_cmp(arg2, "n" ) )
     {
	SPRINTF( buf, "%s has denied %s's name", ch->name,
						       victim->name );
	to_channel( buf, CHANNEL_MONITOR, "Monitor", ch->top_level );
        ch_printf (victim,
          "The MUD Administrators have found the name %s "
          "to be unacceptable.\n"
          "Use 'name' to change it to something more apropriate.\n", victim->name);
	ch_printf( ch, "You requested %s change names.\n", victim->name);
	victim->pcdata->auth_state = 2;
	return;
     }

     else
     {
	send_to_char("Invalid argument.\n", ch);
	return;
     }
}

void do_bamfin( CHAR_DATA *ch, char *argument )
{
    if ( !IS_NPC(ch) )
    {
	smash_tilde( argument );
	STR_DISPOSE( ch->pcdata->bamfin );
	ch->pcdata->bamfin = str_dup( argument );
	send_to_char( "Ok.\n", ch );
    }
    return;
}



void do_bamfout( CHAR_DATA *ch, char *argument )
{
    if ( !IS_NPC(ch) )
    {
	smash_tilde( argument );
	STR_DISPOSE( ch->pcdata->bamfout );
	ch->pcdata->bamfout = str_dup( argument );
	send_to_char( "Ok.\n", ch );
    }
    return;
}

void do_rank( CHAR_DATA *ch, char *argument )
{
  if ( IS_NPC(ch) )
    return;

  if ( !argument || argument[0] == '\0' )
  {
    send_to_char( "Usage: rank <string>.\n", ch );
    send_to_char( "   or: rank none.\n", ch );
    return;
  }

  smash_tilde( argument );
  STR_DISPOSE( ch->pcdata->rank );
  if ( !str_cmp( argument, "none" ) )
    ch->pcdata->rank = str_dup( "" );
  else
    ch->pcdata->rank = str_dup( argument );
  send_to_char( "Ok.\n", ch );

  return;
}


void do_retire( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
	send_to_char( "Retire whom?\n", ch );
	return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n", ch );
	return;
    }

    if ( IS_NPC(victim) )
    {
	send_to_char( "Not on NPC's.\n", ch );
	return;
    }

    if ( get_trust( victim ) >= get_trust( ch ) )
    {
	send_to_char( "You failed.\n", ch );
	return;
    }

    if ( victim->top_level < LEVEL_SAVIOR )
    {
	send_to_char( "The minimum level for retirement is savior.\n", ch );
	return;
    }

    if ( IS_RETIRED( victim ) )
    {
      BV_REMOVE_BIT( victim->pcdata->flags, PCFLAG_RETIRED );
      ch_printf( ch, "%s returns from retirement.\n", victim->name );
      ch_printf( victim, "%s brings you back from retirement.\n", ch->name );
    }
    else
    { 
      BV_SET_BIT( victim->pcdata->flags, PCFLAG_RETIRED );
      ch_printf( ch, "%s is now a retired immortal.\n", victim->name );
      ch_printf( victim, "Courtesy of %s, you are now a retired immortal.\n", ch->name );
    }
    return;
}

void do_deny( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
	send_to_char( "Deny whom?\n", ch );
	return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n", ch );
	return;
    }

    if ( IS_NPC(victim) )
    {
	send_to_char( "Not on NPC's.\n", ch );
	return;
    }

    if ( get_trust( victim ) >= get_trust( ch ) )
    {
	send_to_char( "You failed.\n", ch );
	return;
    }

    BV_SET_BIT(victim->act, PLR_DENY);
    send_to_char( "You are denied access!\n", victim );
    send_to_char( "OK.\n", ch );
    do_quit( victim, "" );

    return;
}



void do_disconnect( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    DESCRIPTOR_DATA *d;
    CHAR_DATA *victim;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
	send_to_char( "Disconnect whom?\n", ch );
	return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n", ch );
	return;
    }

    if ( victim->desc == NULL )
    {
    act( AT_PLAIN, "$N doesn't have a descriptor.", ch, NULL, victim, TO_CHAR );
	return;
    }

    if ( get_trust(ch) <= get_trust( victim ) )
    {
	send_to_char( "They might not like that...\n", ch );
	return;
    }

    for ( d = first_descriptor; d; d = d->next )
    {
	if ( d == victim->desc )
	{
	    close_socket( d, FALSE );
	    send_to_char( "Ok.\n", ch );
	    return;
	}
    }

    bug( "%s: *** desc not found ***.", __func__ );
    send_to_char( "Descriptor not found!\n", ch );
    return;
}

/*
 * Force a level one player to quit.             Gorog
 */
void do_fquit( CHAR_DATA *ch, char *argument ) 
{ 
  CHAR_DATA *victim; 
  char arg1[MAX_INPUT_LENGTH]; 
  argument = one_argument( argument, arg1 ); 
 
  if ( arg1[0] == '\0' ) 
     { 
     send_to_char( "Force whom to quit?\n", ch ); 
     return; 
     } 
 
  if ( !( victim = get_char_world( ch, arg1 ) ) )
     { 
     send_to_char( "They aren't here.\n", ch ); 
     return; 
     } 
 
  if ( victim->top_level != 1 )  
     { 
     send_to_char( "They are not level one!\n", ch ); 
     return; 
     } 
 
  send_to_char( "The MUD administrators force you to quit\n", victim );
  do_quit (victim, "");
  send_to_char( "Ok.\n", ch ); 
  return; 
} 


void do_forceclose( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    DESCRIPTOR_DATA *d;
    int desc;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
	send_to_char( "Usage: forceclose <descriptor#>\n", ch );
	return;
    }
    desc = atoi( arg );

    for ( d = first_descriptor; d; d = d->next )
    {
	if ( d->descriptor == desc )
	{
	    if ( d->character && get_trust(d->character) > get_trust(ch) )
	    {
		send_to_char( "They might not like that...\n", ch );
		return;
	    }
	    close_socket( d, FALSE );
	    send_to_char( "Ok.\n", ch );
	    return;
	}
    }

    send_to_char( "Not found!\n", ch );
    return;
}



void do_pardon( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
	send_to_char( "Syntax: pardon <character> <planet>.\n", ch );
	return;
    }

    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
	send_to_char( "They aren't here.\n", ch );
	return;
    }

    if ( IS_NPC(victim) )
    {
	send_to_char( "Not on NPC's.\n", ch );
	return;
    }

    send_to_char( "Syntax: pardon <character> <planet>.... But it doesn't work .... Tell Durga to hurry up and finish it :p\n", ch );
    return;
}


void echo_to_all( sh_int AT_COLOR, char *argument, sh_int tar )
{
    DESCRIPTOR_DATA *d;

    if ( !argument || argument[0] == '\0' )
	return;

    for ( d = first_descriptor; d; d = d->next )
    {
        /* Added showing echoes to players who are editing, so they won't
           miss out on important info like upcoming reboots. --Narn */ 
	if ( d->connected == CON_PLAYING || d->connected == CON_EDITING )
	{
	    /* This one is kinda useless except for switched.. */
	    if ( tar == ECHOTAR_PC && IS_NPC(d->character) )
	      continue;
	    else if ( tar == ECHOTAR_IMM && !IS_IMMORTAL(d->character) )
	      continue;
	    set_char_color( AT_COLOR, d->character );
	    send_to_char( argument, d->character );
	    send_to_char( "\n",   d->character );
	}
    }
    return;
}

void do_echo( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    sh_int color;
    int target;
    char *parg;

    if ( BV_IS_SET(ch->act, PLR_NO_EMOTE) )
    {
        send_to_char( "You are noemoted and can not echo.\n", ch );
	return;
    }

    if ( argument[0] == '\0' )
    {
	send_to_char( "Echo what?\n", ch );
	return;
    }

    if ( (color = get_color(argument)) )
      argument = one_argument(argument, arg);
    parg = argument;
    argument = one_argument(argument, arg);
    if ( !str_cmp( arg, "PC" )
    ||   !str_cmp( arg, "player" ) )
      target = ECHOTAR_PC;
    else if ( !str_cmp( arg, "imm" ) )
      target = ECHOTAR_IMM;
    else
    {
      target = ECHOTAR_ALL;
      argument = parg;
    }
    if ( !color && (color = get_color(argument)) )
      argument = one_argument(argument, arg);
    if ( !color )
      color = AT_IMMORT;
    one_argument(argument, arg);
    if ( !str_cmp( arg, "Merth" )
    ||   !str_cmp( arg, "Durga" ))
    {
	ch_printf( ch, "I don't think %s would like that!\n", arg );
	return;
    }
    echo_to_all ( color, argument, target );
}

void echo_to_room( sh_int AT_COLOR, ROOM_INDEX_DATA *room, char *argument )
{
    CHAR_DATA *vic;
    
    if ( room == NULL )
    	return;
    	
    
    for ( vic = room->first_person; vic; vic = vic->next_in_room )
    {
	set_char_color( AT_COLOR, vic );
	send_to_char( argument, vic );
	send_to_char( "\n",   vic );
    }
}

void do_recho( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    sh_int color;

    if ( BV_IS_SET(ch->act, PLR_NO_EMOTE) )
    {
        send_to_char( "You are noemoted and can not recho.\n", ch );
	return;
    }

    if ( argument[0] == '\0' )
    {
	send_to_char( "Recho what?\n", ch );
	return;
    }

    one_argument( argument, arg );
    if ( !str_cmp( arg, "Thoric" )
    ||   !str_cmp( arg, "Dominus" )
    ||   !str_cmp( arg, "Circe" )
    ||   !str_cmp( arg, "Haus" )
    ||   !str_cmp( arg, "Narn" )
    ||   !str_cmp( arg, "Scryn" )
    ||   !str_cmp( arg, "Blodkai" )
    ||   !str_cmp( arg, "Damian" ) )
    {
	ch_printf( ch, "I don't think %s would like that!\n", arg );
	return;
    }
    if ( (color = get_color ( argument )) )
       {
       argument = one_argument ( argument, arg );
       echo_to_room ( color, ch->in_room, argument );
       }
    else if( ch->top_level < LEVEL_IMMORTAL )
	echo_to_room( AT_ACTION, ch->in_room, argument );
    else
       echo_to_room ( AT_IMMORT, ch->in_room, argument );
}


ROOM_INDEX_DATA *find_location( CHAR_DATA *ch, char *arg )
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;

    if ( is_number(arg) )
	return get_room_index( atoi( arg ) );

    if ( ( victim = get_char_world( ch, arg ) ) != NULL )
	return victim->in_room;

    if ( ( obj = get_obj_world( ch, arg ) ) != NULL )
	return obj->in_room;

    return NULL;
}

/* This function shared by do_transfer and do_mptransfer
 *
 * Immortals bypass most restrictions on where to transfer victims.
 * NPCs cannot transfer victims who are:
 * 1. Not authorized yet.
 * 2. Outside of the level range for the target room's area.
 * 3. Being sent to private rooms.
 */
void transfer_char( CHAR_DATA *ch, CHAR_DATA *victim, ROOM_INDEX_DATA *location )
{
   if( !victim->in_room )
   {
      bug( "%s: victim in NULL room: %s", __func__, victim->name );
      return;
   }

   if( IS_NPC(ch) && room_is_private( victim, location ) )
   {
      progbug( "Mptransfer - Private room", ch );
      return;
   }

   if( !can_see( ch, victim ) )
      return;

   if( IS_NPC(ch) && NOT_AUTHED( victim ) && location->area != victim->in_room->area )
   {
      char buf[MAX_STRING_LENGTH];

      snprintf( buf, MAX_STRING_LENGTH, "Mptransfer - unauthed char (%s)", victim->name );
      progbug( buf, ch );
      return;
   }

   /* If victim not in area's level range, do not transfer */
   if( IS_NPC(ch) && !in_hard_range( victim, location->area ) && !BV_IS_SET( location->room_flags, ROOM_PROTOTYPE ) )
      return;

   if( victim->fighting )
      stop_fighting( victim, TRUE );

   if( !IS_NPC(ch) )
   {
      act( AT_MAGIC, "$n disappears in a cloud of swirling colors.", victim, NULL, NULL, TO_ROOM );
      victim->retran = victim->in_room->vnum;
   }
   char_from_room( victim );
   char_to_room( victim, location );
   if( !IS_NPC(ch) )
   {
      act( AT_MAGIC, "$n arrives from a puff of smoke.", victim, NULL, NULL, TO_ROOM );
      if( ch != victim )
         act( AT_IMMORT, "$n has transferred you.", ch, NULL, victim, TO_VICT );
      do_look( victim, "auto" );
      if( !IS_IMMORTAL( victim ) && !IS_NPC( victim ) && !in_hard_range( victim, location->area ) )
         act( AT_DANGER, "Warning:  this player's level is not within the area's level range.", ch, NULL, NULL, TO_CHAR );
   }
}

void do_transfer( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;
    DESCRIPTOR_DATA *d;
    CHAR_DATA *victim;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' )
    {
	send_to_char( "Transfer whom (and where)?\n", ch );
	return;
    }

    /*
     * Thanks to Grodyn for the optional location parameter.
     */
    if ( arg2[0] == '\0' )
    {
	    location = ch->in_room;
    }
    else
    {
	if ( ( location = find_location( ch, arg2 ) ) == NULL )
	{
	    send_to_char( "That location does not exist.\n", ch );
	    return;
	}

    if ( !str_cmp( arg1, "all" ) && get_trust( ch ) >= LEVEL_GREATER )
    {
        for( d = first_descriptor; d; d = d->next )
        {
            if( d->connected == CON_PLAYING && d->character && d->character != ch && d->character->in_room )
                transfer_char( ch, d->character, location );
        }
        return;
    }

	if ( room_is_private( ch, location ) )
	{
	    send_to_char( "That room is private right now.\n", ch );
	    return;
	}
    }

    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
	send_to_char( "They aren't here.\n", ch );
	return;
    }

    if (NOT_AUTHED(victim))
    {
	send_to_char( "They are not authorized yet!\n", ch);
	return;
    }

    if ( !victim->in_room )
    {
	send_to_char( "They are in limbo.\n", ch );
	return;
    }

   transfer_char( ch, victim, location );
}

void do_retran( CHAR_DATA *ch, char *argument )
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	char buf[MAX_STRING_LENGTH];
	
	argument = one_argument( argument, arg );
	if ( arg[0] == '\0' )
	{
		send_to_char("Retransfer whom?\n", ch );
		return;
	}
	if ( !(victim = get_char_world(ch, arg)) )
	{
		send_to_char("They aren't here.\n", ch );
		return;
	}
	SPRINTF(buf, "'%s' %d", victim->name, victim->retran);
	do_transfer(ch, buf);
	return;
}

void do_regoto( CHAR_DATA *ch, char *argument )
{
	char buf[MAX_STRING_LENGTH];
	
	SPRINTF(buf, "%d", ch->regoto);
	do_goto(ch, buf);
	return;
}

void do_scatter( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    char arg[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *pRoomIndex;

    set_char_color( AT_IMMORT, ch );

    one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
      send_to_char( "Scatter whom?\n", ch );
      return; }
    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
      send_to_char( "They aren't here.\n", ch );
      return; }
    if ( victim == ch ) {
      send_to_char( "It's called teleport.  Try it.\n", ch );
      return;
    }
    if ( !IS_NPC(victim) && get_trust( victim ) >= get_trust( ch ) ) {
      send_to_char( "You haven't the power to succeed against them.\n", ch );
      return; }
    for ( ; ; ) {
      pRoomIndex = get_room_index( number_range( 0, 32767 ) );
      if ( pRoomIndex )
      if ( !BV_IS_SET(pRoomIndex->room_flags, ROOM_PRIVATE)
      &&   !BV_IS_SET(pRoomIndex->room_flags, ROOM_SOLITARY)
      &&   !BV_IS_SET(pRoomIndex->room_flags, ROOM_PROTOTYPE)
      &&   !BV_IS_SET(pRoomIndex->room_flags, ROOM_SPACECRAFT) )
      break; }
    if ( victim->fighting ) stop_fighting( victim, TRUE );
    act( AT_MAGIC, "With the sweep of an arm, $n flings $N to the winds.", ch, NULL, victim, TO_NOTVICT );
    act( AT_MAGIC, "With the sweep of an arm, $n flings you to the astral winds.", ch, NULL, victim, TO_VICT );
    act( AT_MAGIC, "With the sweep of an arm, you fling $N to the astral winds.", ch, NULL, victim, TO_CHAR );
    char_from_room( victim );
    char_to_room( victim, pRoomIndex );
    victim->position = POS_RESTING;
    act( AT_MAGIC, "$n staggers forth from a sudden gust of wind, and collapses.", victim, NULL, NULL, TO_ROOM );
    do_look( victim, "auto" );
    return;
}

void do_mortalize( CHAR_DATA *ch, char *argument )
{
	return;
}
void do_delay( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    char arg[MAX_INPUT_LENGTH];
    int delay;

    set_char_color( AT_IMMORT, ch );
 
    argument = one_argument( argument, arg );
    if ( !*arg ) {
      send_to_char( "Syntax:  delay <victim> <# of rounds>\n", ch );
      return;
    }
    if ( !( victim = get_char_world( ch, arg ) ) ) {
      send_to_char( "No such character online.\n", ch );
      return;
    }
    if ( IS_NPC( victim ) ) {
      send_to_char( "Mobiles are unaffected by lag.\n", ch );
      return;
    }
    if ( !IS_NPC(victim) && get_trust( victim ) >= get_trust( ch ) ) {
      send_to_char( "You haven't the power to succeed against them.\n", ch );
      return;
    }
    argument = one_argument(argument, arg);
    if ( !*arg ) {
      send_to_char( "For how long do you wish to delay them?\n", ch );
      return;
    }
    if ( !str_cmp( arg, "none" ) ) {
      send_to_char( "All character delay removed.\n", ch );
      victim->wait = 0;
      return;
    }
    delay = atoi( arg );
    if ( delay < 1 ) {
      send_to_char( "Pointless.  Try a positive number.\n", ch );
      return;
    }
    if ( delay > 999 ) {
      send_to_char( "You cruel bastard.  Just kill them.\n", ch );
      return;
    }
    WAIT_STATE( victim, delay * PULSE_VIOLENCE );
    ch_printf( ch, "You've delayed %s for %d rounds.\n", victim->name, delay );
    return;
}


	
void do_at( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;
    ROOM_INDEX_DATA *original;
    CHAR_DATA *wch;

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' || argument[0] == '\0' )
    {
	send_to_char( "At where what?\n", ch );
	return;
    }

    if ( ( location = find_location( ch, arg ) ) == NULL )
    {
	send_to_char( "No such location.\n", ch );
	return;
    }

      if ( get_trust( ch ) < LEVEL_GREATER )
      {
        if ( room_is_private( ch, location ) )
        {
	  send_to_char( "That room is private right now.\n", ch );
	  return;
        }
      }
      if ( check_for_immroom( ch, location ) )
      {
	send_to_char( "That room is private right now.\n", ch );
	return;
      }
      if ( room_is_private( ch, location ) )
        send_to_char( "Overriding private flag!\n", ch );
      

    original = ch->in_room;
    char_from_room( ch );
    char_to_room( ch, location );
    interpret( ch, argument );

    /*
     * See if 'ch' still exists before continuing!
     * Handles 'at XXXX quit' case.
     */
    for ( wch = first_char; wch; wch = wch->next )
    {
	if ( wch == ch )
	{
	    char_from_room( ch );
	    char_to_room( ch, original );
	    break;
	}
    }

    return;
}

void do_rat( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;
    ROOM_INDEX_DATA *original;
    int Start, End, vnum;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' || argument[0] == '\0' )
    {
	send_to_char( "Syntax: rat <start> <end> <command>\n", ch );
	return;
    }

    Start = atoi( arg1 );	End = atoi( arg2 );

    if ( Start < 1 || End < Start || Start > End || Start == End || End > 32767 )
    {
	send_to_char( "Invalid range.\n", ch );
	return;
    }

    if ( !str_cmp( argument, "quit" ) )
    {
	send_to_char( "I don't think so!\n", ch );
	return;
    }

    original = ch->in_room;
    for ( vnum = Start; vnum <= End; vnum++ )
    {
	if ( (location = get_room_index(vnum)) == NULL )
	  continue;
	char_from_room( ch );
	char_to_room( ch, location );
	interpret( ch, argument );
    }

    char_from_room( ch );
    char_to_room( ch, original );
    send_to_char( "Done.\n", ch );
    return;
}


void do_rstat( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;
    OBJ_DATA *obj;
    CHAR_DATA *rch;
    EXIT_DATA *pexit;
    int cnt;
    static char *dir_text[] = { "n", "e", "s", "w", "u", "d", "ne", "nw", "se", "sw", "?" };

    one_argument( argument, arg );
    
    if ( get_trust( ch ) < LEVEL_IMMORTAL )
    {
        AREA_DATA * pArea;

        if ( !ch->pcdata || !(pArea=ch->pcdata->area) )
        {
  	   send_to_char( "You must have an assigned area to goto.\n", ch );
	   return;
        }
        
        if ( ch->in_room->vnum < pArea->low_r_vnum
	||  ch->in_room->vnum > pArea->hi_r_vnum )
	{
	    send_to_char( "You can only rstat within your assigned range.\n", ch );
	    return;
	}
	
    }
    
    
    if ( !str_cmp( arg, "exits" ) )
    {
	location = ch->in_room;

	ch_printf( ch, "Exits for room '%s.' vnum %d\n",
		location->name,
		location->vnum );

	for ( cnt = 0, pexit = location->first_exit; pexit; pexit = pexit->next )
	    ch_printf( ch,
		"%2d) %2s to %-5d.  Key: %d  Flags: %d  Keywords: '%s'.\nDescription: %sExit links back to vnum: %d  Exit's RoomVnum: %d  Distance: %d\n",
		++cnt,
		dir_text[pexit->vdir],
		pexit->to_room ? pexit->to_room->vnum : 0,
		pexit->key,
		pexit->exit_info,
		pexit->keyword,
		pexit->description[0] != '\0'
		    ? pexit->description : "(none).\n",
		pexit->rexit ? pexit->rexit->vnum : 0,
		pexit->rvnum,
		pexit->distance );
	return;
    }
    location = ( arg[0] == '\0' ) ? ch->in_room : find_location( ch, arg );
    if ( !location )
    {
	send_to_char( "No such location.\n", ch );
	return;
    }

    if ( ch->in_room != location && room_is_private( ch, location ) )
    {
      if ( get_trust( ch ) < LEVEL_GREATER )
      {
        send_to_char( "That room is private right now.\n", ch );
        return;
      }
      else
      {
        send_to_char( "Overriding private flag!\n", ch );
      }

    }

    ch_printf( ch, "Name: %s.\nArea: %s  Filename: %s.\n",
	location->name,
	location->area ? location->area->name : "None????",
	location->area ? location->area->filename : "None????" );

    ch_printf( ch,
	"Vnum: %d.  Sector: %d.  Light: %d.  TeleDelay: %d.  TeleVnum: %d  Tunnel: %d.\n",
	location->vnum,
	location->sector_type,
	location->light,
	location->tele_delay,
	location->tele_vnum,
	location->tunnel );

    ch_printf( ch, "Room flags: %s\n", bitset_to_string(location->room_flags, r_flags).c_str());
	ch_printf( ch, "Description:\n%s", location->description );

    if ( location->first_extradesc )
    {
	EXTRA_DESCR_DATA *ed;

	send_to_char( "Extra description keywords: '", ch );
	for ( ed = location->first_extradesc; ed; ed = ed->next )
	{
	    send_to_char( ed->keyword, ch );
	    if ( ed->next )
		send_to_char( " ", ch );
	}
	send_to_char( "'.\n", ch );
    }

    send_to_char( "Characters:", ch );
    for ( rch = location->first_person; rch; rch = rch->next_in_room )
    {
	if ( can_see( ch, rch ) )
	{
	  send_to_char( " ", ch );
	  one_argument( rch->name, buf );
	  send_to_char( buf, ch );
	}
    }

    send_to_char( ".\nObjects:   ", ch );
    for ( obj = location->first_content; obj; obj = obj->next_content )
    {
	send_to_char( " ", ch );
	one_argument( obj->name, buf );
	send_to_char( buf, ch );
    }
    send_to_char( ".\n", ch );

    if ( location->first_exit )
	send_to_char( "------------------- EXITS -------------------\n", ch );
    for ( cnt = 0, pexit = location->first_exit; pexit; pexit = pexit->next )
	ch_printf( ch,
	  "%2d) %-2s to %-5d.  Key: %d  Flags: %d  Keywords: %s.\n",
		++cnt,
		dir_text[pexit->vdir],
		pexit->to_room ? pexit->to_room->vnum : 0,
		pexit->key,
		pexit->exit_info,
		pexit->keyword[0] != '\0' ? pexit->keyword : "(none)" );
    return;
}



void do_ostat( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    AFFECT_DATA *paf;
    OBJ_DATA *obj;
    char *pdesc;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Ostat what?\n", ch );
	return;
    }
    if ( arg[0] != '\'' && arg[0] != '"' && strlen(argument) > strlen(arg) )
	STRLCPY( arg, argument );

    if ( ( obj = get_obj_world( ch, arg ) ) == NULL )
    {
	send_to_char( "Nothing like that in hell, earth, or heaven.\n", ch );
	return;
    }

    ch_printf( ch, "Name: %s.\n",
	obj->name );
    
    pdesc=get_extra_descr(arg, obj->first_extradesc);
    if ( !pdesc )
       pdesc=get_extra_descr(arg, obj->pIndexData->first_extradesc);
    if ( !pdesc )	    
       pdesc = get_extra_descr( obj->name, obj->first_extradesc );
    if ( !pdesc )
       pdesc = get_extra_descr( obj->name, obj->pIndexData->first_extradesc );
    if ( pdesc )
       send_to_char( pdesc, ch );
		
    
    ch_printf( ch, "Vnum: %d.  Type: %s.  Count: %d  Gcount: %d\n",
	obj->pIndexData->vnum, item_type_name( obj ), obj->pIndexData->count,
	obj->count );

    ch_printf( ch, "Serial#: %d  TopIdxSerial#: %d  TopSerial#: %d\n",
	obj->serial, obj->pIndexData->serial, cur_obj_serial );

    ch_printf( ch, "Short description: %s.\nLong description: %s\n",
	obj->short_descr, obj->description );

    if ( obj->action_desc[0] != '\0' )
	ch_printf( ch, "Action description: %s.\n", obj->action_desc );

    ch_printf( ch, "Wear flags : %s : Proto: %s\n", 
        bitset_to_string(obj->wear_flags, w_flags).c_str(), 
        bitset_to_string(obj->pIndexData->wear_flags, w_flags).c_str() );

    ch_printf( ch, "Number: %d/%d.  Weight: %d/%d.  Layers: %d\n",
	1,           get_obj_number( obj ),
	obj->weight, get_obj_weight( obj ), obj->pIndexData->layers );

    ch_printf( ch, "Cost: %d.  Rent: %d.  Timer: %d.  Level: %d.\n",
	obj->cost, obj->pIndexData->rent, obj->timer, obj->level );

    ch_printf( ch,
	"In room: %d.  In object: %s.  Carried by: %s.  Wear_loc: %d.\n",
	obj->in_room    == NULL    ?        0 : obj->in_room->vnum,
	obj->in_obj     == NULL    ? "(none)" : obj->in_obj->short_descr,
	obj->carried_by == NULL    ? "(none)" : obj->carried_by->name,
	obj->wear_loc );

    ch_printf( ch, "Index Values : %d %d %d %d %d %d.\n",
	obj->pIndexData->value[0], obj->pIndexData->value[1],
	obj->pIndexData->value[2], obj->pIndexData->value[3],
	obj->pIndexData->value[4], obj->pIndexData->value[5] );
    ch_printf( ch, "Object Values: %d %d %d %d %d %d.\n",
	obj->value[0], obj->value[1], obj->value[2], obj->value[3], obj->value[4], obj->value[5] );
    ch_printf(ch, "Object Flags: %s\n", bitset_to_string(obj->objflags,obj_flag_table).c_str());
    ch_printf(ch, "Index Object Flags: %s\n", bitset_to_string(obj->pIndexData->objflags,obj_flag_table).c_str());

    if ( obj->pIndexData->first_extradesc )
    {
	EXTRA_DESCR_DATA *ed;

	send_to_char( "Primary description keywords:   '", ch );
	for ( ed = obj->pIndexData->first_extradesc; ed; ed = ed->next )
	{
	    send_to_char( ed->keyword, ch );
	    if ( ed->next )
		send_to_char( " ", ch );
	}
	send_to_char( "'.\n", ch );
    }
    if ( obj->first_extradesc )
    {
	EXTRA_DESCR_DATA *ed;

	send_to_char( "Secondary description keywords: '", ch );
	for ( ed = obj->first_extradesc; ed; ed = ed->next )
	{
	    send_to_char( ed->keyword, ch );
	    if ( ed->next )
		send_to_char( " ", ch );
	}
	send_to_char( "'.\n", ch );
    }

    for ( paf = obj->first_affect; paf; paf = paf->next )
	ch_printf( ch, "Affects %s by %d. (extra)\n",
	    affect_loc_name( paf->location ), paf->modifier );

    for ( paf = obj->pIndexData->first_affect; paf; paf = paf->next )
	ch_printf( ch, "Affects %s by %d.\n",
	    affect_loc_name( paf->location ), paf->modifier );

    return;
}



void do_mstat( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    AFFECT_DATA *paf;
    CHAR_DATA *victim;
    SKILLTYPE *skill;
    int x;

    set_char_color( AT_PLAIN, ch );

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
	send_to_char( "Mstat whom?\n", ch );
	return;
    }
    if ( arg[0] != '\'' && arg[0] != '"' && strlen(argument) > strlen(arg) )
	STRLCPY( arg, argument );

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n", ch );
	return;
    }
    if ( ( ( get_trust( ch ) < LEVEL_GOD ) && !IS_NPC(victim) ) || ( ( get_trust( ch ) < get_trust( victim ) ) && !IS_NPC(victim) ) )
    {
	set_char_color( AT_IMMORT, ch );
	send_to_char( "Their godly glow prevents you from getting a good look.\n", ch );
	return;
    }
    ch_printf( ch, "Name: %s     Organization: %s\n",
	victim->name,
	( IS_NPC( victim ) || !victim->pcdata->clan ) ? "(none)" 
			       : victim->pcdata->clan->name );
    if( get_trust(ch) >= LEVEL_GOD && !IS_NPC(victim) && victim->desc )
	ch_printf( ch, "User: %s@%s   Descriptor: %d   Trust: %d   AuthedBy: %s\n",
		victim->desc->user, victim->desc->host, victim->desc->descriptor,
		victim->trust, victim->pcdata->authed_by[0] != '\0'
		? victim->pcdata->authed_by : "(unknown)" );
    if ( !IS_NPC(victim) && victim->pcdata->release_date != 0 )
      ch_printf(ch, "Helled until %24.24s by %s.\n",
              ctime(&victim->pcdata->release_date),
              victim->pcdata->helled_by);

    ch_printf( ch, "Vnum: %d  Sex: %s  Room: %d  PHome: %d  Count: %d  Killed: %d\n",
	IS_NPC(victim) ? victim->pIndexData->vnum : 0,
	victim->sex == SEX_MALE    ? "male"   :
	victim->sex == SEX_FEMALE  ? "female" : "neutral",
	victim->in_room == NULL    ?        0 : victim->in_room->vnum,
	IS_NPC(victim) ? -1 : (victim->plr_home ? victim->plr_home->vnum : 0 ),
	IS_NPC(victim) ? victim->pIndexData->count : 1,
	IS_NPC(victim) ? victim->pIndexData->killed
		       : victim->pcdata->mdeaths + victim->pcdata->pdeaths
	);
    ch_printf( ch, "Str: %d  Int: %d  Wis: %d  Dex: %d  Con: %d  Cha: %d  Lck: %d  Frc: %d\n",
	get_curr_str(victim),
	get_curr_int(victim),
	get_curr_wis(victim),
	get_curr_dex(victim),
	get_curr_con(victim),
	get_curr_cha(victim),
	get_curr_lck(victim),
	get_curr_frc(victim) );
    ch_printf( ch, "Hps: %d/%d  Force: %d/%d   Move: %d/%d\n",
        victim->hit,         victim->max_hit,
        victim->mana,        victim->max_mana,
        victim->move,        victim->max_move );  
    if ( !IS_NPC( victim ) )
    { 
       int ability;
    
       for ( ability = 0 ; ability < MAX_ABILITY ; ability++ )
            ch_printf( ch, "%-15s   Level: %-3d   Max: %-3d   Exp: %-10ld   Next: %-10ld\n",
            ability_name[ability], victim->skill_level[ability], max_level(victim, ability), victim->experience[ability],
            exp_level( victim->skill_level[ability]+1 ) );
    }
ch_printf( ch,
	"Top Level: %d     Race: %d  Align: %d  AC: %d  Gold: %d  Bank: %d\n",
	victim->top_level,  victim->race,   victim->alignment,
	GET_AC(victim),      victim->gold,
	victim->pcdata ? victim->pcdata->bank : -1 );
    ch_printf(ch,"Command Groups: ");
    {
      int i;
      for(i = 0; i < MAX_COMMAND_GROUP; i++) {
        if (!IS_NPC(victim) && BV_IS_SET(victim->pcdata->commandgroup,(1<<i)))
          ch_printf(ch,"%s ",get_flag_name(command_groups, i, MAX_COMMAND_GROUP));
      }
    }
    ch_printf(ch,"\n");
    if (  victim->race  < MAX_NPC_RACE  && victim->race  >= 0 )
	ch_printf( ch, "Race: %s\n",
	  get_flag_name(npc_race, victim->race, MAX_NPC_RACE) );
    ch_printf( ch, "Hitroll: %d   Damroll: %d   Position: %d   Wimpy: %d \n",
	GET_HITROLL(victim), GET_DAMROLL(victim),
	victim->position,    victim->wimpy );
    ch_printf( ch, "Fighting: %s    Master: %s    Leader: %s\n",
	victim->fighting ? victim->fighting->who->name : "(none)",
        victim->master      ? victim->master->name   : "(none)",
        victim->leader      ? victim->leader->name   : "(none)" );
    ch_printf( ch, "Questpoints: %d    Current QuestMob: %d    Current QuestObj: %d\n",
        victim->questpoints, victim->questmob, victim->questobj );    
    
    if ( !IS_NPC(victim) )
	ch_printf( ch,
	    "Thirst: %d   Full: %d   Drunk: %d     Glory: %d/%d\n",
	    victim->pcdata->condition[COND_THIRST],
	    victim->pcdata->condition[COND_FULL],
	    victim->pcdata->condition[COND_DRUNK],
	    victim->pcdata->quest_curr,
	    victim->pcdata->quest_accum );
    else
	ch_printf( ch, "Hit dice: %dd%d+%d.  Damage dice: %dd%d+%d.\n",
		victim->pIndexData->hitnodice,
		victim->pIndexData->hitsizedice,
		victim->pIndexData->hitplus,
		victim->pIndexData->damnodice,
		victim->pIndexData->damsizedice,
		victim->pIndexData->damplus );
    ch_printf( ch, "MentalState: %d   EmotionalState: %d\n",
 		victim->mental_state, victim->emotional_state );
    ch_printf( ch, "Saving throws: %d %d %d %d %d.\n",
		victim->saving_poison_death,
		victim->saving_wand,
		victim->saving_para_petri,
		victim->saving_breath,
		victim->saving_spell_staff  );
    ch_printf( ch, "Carry figures: items (%d/%d)  weight (%d/%d)   Numattacks: %d\n",
	victim->carry_number, can_carry_n(victim), victim->carry_weight, can_carry_w(victim), victim->numattacks );
    if ( IS_NPC( victim ) )
    {	
	ch_printf( ch, "Act flags: %s\n", bitset_to_string(victim->act, act_flags).c_str() );
        ch_printf( ch, "VIP flags: %s\n", bitset_to_string(victim->vip_flags, planet_flags).c_str() );
    }    
    else
    { 
        ch_printf( ch, "Years: %d   Seconds Played: %d   Timer: %d   Act: %d\n",
	    get_age( victim ), (int) victim->pcdata->played, victim->timer, victim->act );
	
	ch_printf( ch, "Player flags: %s\n",
		bitset_to_string(victim->act, plr_flags).c_str() );
	ch_printf( ch, "Pcflags: %s\n",
		bitset_to_string(victim->pcdata->flags, pc_flags).c_str() );
	ch_printf( ch, "Wanted flags: %s\n",
		bitset_to_string(victim->pcdata->wanted_flags, planet_flags).c_str() );
    }
    ch_printf( ch, "Affected by: %s\n",
	    bitset_to_string(victim->affected_by, aff_flags).c_str());
    ch_printf( ch, "Speaking: %d\n", victim->speaking );
    send_to_char("Languages: ", ch);

    for (x = 0; x < LANG_MAX; ++x)
    {
        if (!lang_names[x].name)
            continue;
        bool knows = knows_language(victim, x, victim);
        bool speaking = (victim->speaking == x);

        if (knows)
        {
            if (speaking)
                set_char_color(AT_RED, ch);
            send_to_char(lang_names[x].name, ch);
            send_to_char(" ", ch);
            set_char_color(AT_PLAIN, ch);
        }
        else if (speaking)
        {
            set_char_color(AT_PINK, ch);
            send_to_char(lang_names[x].name, ch);
            send_to_char(" ", ch);
            set_char_color(AT_PLAIN, ch);
        }
    }
    send_to_char("\n", ch);
    if ( victim->pcdata && victim->pcdata->bestowments 
         && victim->pcdata->bestowments[0] != '\0' )
      ch_printf( ch, "Bestowments: %s\n", victim->pcdata->bestowments );
    ch_printf( ch, "Short description: %s\nLong  description: %s",
	victim->short_descr,
	victim->long_descr[0] != '\0' ? victim->long_descr : "(none)\n" );
    if ( IS_NPC(victim) && ( victim->spec_fun || victim->spec_2 ) )
	ch_printf( ch, "Mobile has spec fun: %s %s\n",
		lookup_spec( victim->spec_fun ),
		victim->spec_2 ? lookup_spec( victim->spec_2 ) : "" );
    ch_printf( ch, "Body Parts : %s\n",
	flag_string(victim->xflags, part_flags) );
    ch_printf( ch, "Resistant  : %s\n",
	flag_string(victim->resistant, ris_flags) );
    ch_printf( ch, "Immune     : %s\n",
	flag_string(victim->immune, ris_flags) );
    ch_printf( ch, "Susceptible: %s\n",
	flag_string(victim->susceptible, ris_flags) );
    ch_printf( ch, "Attacks    : %s\n",
	flag_string(victim->attacks, attack_flags) );
    ch_printf( ch, "Defenses   : %s\n",
	flag_string(victim->defenses, defense_flags) );
    for ( paf = victim->first_affect; paf; paf = paf->next )
    {
        skill=get_skilltype(paf->type);
        {
        ch_printf( ch,
            "%s: '%s' modifies %s by %d(%s) for %d rounds",
            skill ? get_flag_name(skill_tname,skill->type,SKILL_MAX) : "none",
            skill ? skill->name : "none",
            affect_loc_name( paf->location ),
            paf->modifier,
            get_flag_name(aff_flags, paf->modifier, AFF_MAX),
            paf->duration
            );
            if (paf->bitvector < 0 || paf->bitvector >= AFF_MAX)
                ch_printf(ch, ".\n", paf->bitvector);
            else
                ch_printf(ch, " with bits %s.\n", aff_flags[paf->bitvector].name);
        }
    }   
    return;
}



void do_mfind( CHAR_DATA *ch, char *argument )
{
/*  extern int top_mob_index; */
    char arg[MAX_INPUT_LENGTH];
    MOB_INDEX_DATA *pMobIndex;
/*  int vnum; */
    int hash;
    int nMatch;
    bool fAll;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
	send_to_char( "Mfind whom?\n", ch );
	return;
    }

    fAll	= !str_cmp( arg, "all" );
    nMatch	= 0;
    set_pager_color( AT_PLAIN, ch );

    /*
     * Yeah, so iterating over all vnum's takes 10,000 loops.
     * Get_mob_index is fast, and I don't feel like threading another link.
     * Do you?
     * -- Furey
     */
/*  for ( vnum = 0; nMatch < top_mob_index; vnum++ )
    {
	if ( ( pMobIndex = get_mob_index( vnum ) ) != NULL )
	{
	    if ( fAll || is_name( arg, pMobIndex->player_name ) )
	    {
		nMatch++;
		SPRINTF( buf, "[%5d] %s\n",
		    pMobIndex->vnum, capitalize( pMobIndex->short_descr ) );
		send_to_char( buf, ch );
	    }
	}
    }
     */

    /*
     * This goes through all the hash entry points (1024), and is therefore
     * much faster, though you won't get your vnums in order... oh well. :)
     *
     * Tests show that Furey's method will usually loop 32,000 times, calling
     * get_mob_index()... which loops itself, an average of 1-2 times...
     * So theoretically, the above routine may loop well over 40,000 times,
     * and my routine bellow will loop for as many index_mobiles are on
     * your mud... likely under 3000 times.
     * -Thoric
     */
    for ( hash = 0; hash < MAX_KEY_HASH; hash++ )
	for ( pMobIndex = mob_index_hash[hash];
	      pMobIndex;
	      pMobIndex = pMobIndex->next )
	    if ( fAll || nifty_is_name( arg, pMobIndex->player_name ) )
	    {
		nMatch++;
		pager_printf( ch, "[%5d] %s\n",
		    pMobIndex->vnum, capitalize( pMobIndex->short_descr ) );
	    }

    if ( nMatch )
	pager_printf( ch, "Number of matches: %d\n", nMatch );
    else
	send_to_char( "Nothing like that in hell, earth, or heaven.\n", ch );

    return;
}



void do_ofind( CHAR_DATA *ch, char *argument )
{
/*  extern int top_obj_index; */
    char arg[MAX_INPUT_LENGTH];
    OBJ_INDEX_DATA *pObjIndex;
/*  int vnum; */
    int hash;
    int nMatch;
    bool fAll;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
	send_to_char( "Ofind what?\n", ch );
	return;
    }

    set_pager_color( AT_PLAIN, ch );
    fAll	= !str_cmp( arg, "all" );
    nMatch	= 0;
/*  nLoop	= 0; */

    /*
     * Yeah, so iterating over all vnum's takes 10,000 loops.
     * Get_obj_index is fast, and I don't feel like threading another link.
     * Do you?
     * -- Furey
    for ( vnum = 0; nMatch < top_obj_index; vnum++ )
    {
	nLoop++;
	if ( ( pObjIndex = get_obj_index( vnum ) ) != NULL )
	{
	    if ( fAll || nifty_is_name( arg, pObjIndex->name ) )
	    {
		nMatch++;
		SPRINTF( buf, "[%5d] %s\n",
		    pObjIndex->vnum, capitalize( pObjIndex->short_descr ) );
		send_to_char( buf, ch );
	    }
	}
    }
     */

    /*
     * This goes through all the hash entry points (1024), and is therefore
     * much faster, though you won't get your vnums in order... oh well. :)
     *
     * Tests show that Furey's method will usually loop 32,000 times, calling
     * get_obj_index()... which loops itself, an average of 2-3 times...
     * So theoretically, the above routine may loop well over 50,000 times,
     * and my routine bellow will loop for as many index_objects are on
     * your mud... likely under 3000 times.
     * -Thoric
     */
    for ( hash = 0; hash < MAX_KEY_HASH; hash++ )
	for ( pObjIndex = obj_index_hash[hash];
	      pObjIndex;
	      pObjIndex = pObjIndex->next )
	    if ( fAll || nifty_is_name( arg, pObjIndex->name ) )
	    {
		nMatch++;
		pager_printf( ch, "[%5d] %s\n",
		    pObjIndex->vnum, capitalize( pObjIndex->short_descr ) );
	    }

    if ( nMatch )
	pager_printf( ch, "Number of matches: %d\n", nMatch );
    else
	send_to_char( "Nothing like that in hell, earth, or heaven.\n", ch );

    return;
}



void do_mwhere( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    bool found;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
	send_to_char( "Mwhere whom?\n", ch );
	return;
    }

    set_pager_color( AT_PLAIN, ch );
    found = FALSE;
    for ( victim = first_char; victim; victim = victim->next )
    {
	if ( IS_NPC(victim)
	&&   victim->in_room
	&&   nifty_is_name( arg, victim->name ) )
	{
	    found = TRUE;
	    pager_printf( ch, "[%5d] %-28s [%5d] %s\n",
		victim->pIndexData->vnum,
		victim->short_descr,
		victim->in_room->vnum,
		victim->in_room->name );
	}
    }

    if ( !found )
	act( AT_PLAIN, "You didn't find any $T.", ch, NULL, arg, TO_CHAR );

    return;
}


void do_bodybag( CHAR_DATA *ch, char *argument )
{
    char buf2[MAX_STRING_LENGTH];
    char buf3[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    bool found;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
	send_to_char( "Bodybag whom?\n", ch );
	return;
    }

    /* make sure the buf3 is clear? */
    SPRINTF(buf3, " ");
    /* check to see if vict is playing? */
    SPRINTF(buf2,"the corpse of %s",arg); 
    found = FALSE;
    for ( obj = first_object; obj; obj = obj->next )
    {
        if( obj->in_room && obj->pIndexData->vnum == OBJ_VNUM_CORPSE_PC && !str_cmp( buf2, obj->short_descr ) )
        {
            found = TRUE;
            ch_printf( ch, "Bagging body: [%5d] %-28s [%5d] %s\n",
            obj->pIndexData->vnum,
            obj->short_descr,
            obj->in_room->vnum,
            obj->in_room->name );
                obj_from_room(obj); 
                obj = obj_to_char(obj, ch);
            obj->timer = -1;
                save_char_obj( ch );
        }
    }

    if ( !found )
	ch_printf(ch," You couldn't find any %s\n",buf2);
    return;
}

#if 0
/* New owhere by Altrag, 03/14/96 */
void do_owhere( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    bool found;
    int icnt = 0;

    argument = one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
	send_to_char( "Owhere what?\n", ch );
	return;
    }
    argument = one_argument(argument, arg1);
    
    set_pager_color( AT_PLAIN, ch );
    if ( arg1[0] != '\0' && !str_prefix(arg1, "nesthunt") )
    {
      if ( !(obj = get_obj_world(ch, arg)) )
      {
        send_to_char( "Nesthunt for what object?\n", ch );
        return;
      }
      for ( ; obj->in_obj; obj = obj->in_obj )
      {
	pager_printf(ch, "[%5d] %-28s in object [%5d] %s\n",
                obj->pIndexData->vnum, obj_short(obj),
                obj->in_obj->pIndexData->vnum, obj->in_obj->short_descr);
	++icnt;
      }
      SPRINTF(buf, "[%5d] %-28s in ", obj->pIndexData->vnum,
		obj_short(obj));
      if ( obj->carried_by )
        SPRINTF(buf+strlen(buf), "invent [%5d] %s\n",
                (IS_NPC(obj->carried_by) ? obj->carried_by->pIndexData->vnum
                : 0), PERS(obj->carried_by, ch));
      else if ( obj->in_room )
        SPRINTF(buf+strlen(buf), "room   [%5d] %s\n",
                obj->in_room->vnum, obj->in_room->name);
      else if ( obj->in_obj )
      {
        bug("do_owhere: obj->in_obj after NULL!",0);
        STRAPP(buf, "object??\n");
      }
      else
      {
        bug("do_owhere: object doesnt have location!",0);
        STRAPP(buf, "nowhere??\n");
      }
      send_to_pager(buf, ch);
      ++icnt;
      pager_printf(ch, "Nested %d levels deep.\n", icnt);
      return;
    }

    found = FALSE;
    for ( obj = first_object; obj; obj = obj->next )
    {
        if ( !nifty_is_name( arg, obj->name ) )
            continue;
        found = TRUE;
        
        SPRINTF(buf, "(%3d) [%5d] %-28s in ", ++icnt, obj->pIndexData->vnum,
                obj_short(obj));
        if ( obj->carried_by )
          SPRINTF(buf+strlen(buf), "invent [%5d] %s\n",
                  (IS_NPC(obj->carried_by) ? obj->carried_by->pIndexData->vnum
                  : 0), PERS(obj->carried_by, ch));
        else if ( obj->in_room )
          SPRINTF(buf+strlen(buf), "room   [%5d] %s\n",
                  obj->in_room->vnum, obj->in_room->name);
        else if ( obj->in_obj )
          SPRINTF(buf+strlen(buf), "object [%5d] %s\n",
                  obj->in_obj->pIndexData->vnum, obj_short(obj->in_obj));
        else
        {
          bug("do_owhere: object doesnt have location!",0);
          STRAPP(buf, "nowhere??\n");
        }
        send_to_pager(buf, ch);
    }

    if ( !found )
      act( AT_PLAIN, "You didn't find any $T.", ch, NULL, arg, TO_CHAR );
    else
      pager_printf(ch, "%d matches.\n", icnt);

    return;
}
#endif

void trunc1(char *s, int len)
{
   if ( strlen(s) > (unsigned int ) len )
      s[len] = '\0';
}

void do_owhere( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH], field[MAX_INPUT_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj, *outer_obj;
    bool found = FALSE;
    int icnt=0, vnum=0;
    char heading[] =
"    Vnum  Short Desc        Vnum  Room/Char          Vnum  Container\n";

    argument = one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
        pager_printf( ch, "Owhere what?\n" );
        return;
    }
    if ( is_number(arg) )
       vnum=atoi(arg);

    for ( obj = first_object; obj; obj = obj->next )
    {
        if ( vnum )
        {  
           if ( vnum!=obj->pIndexData->vnum)
              continue;
        }   
        else if ( !nifty_is_name( arg, obj->name ) )
              continue;

        if ( !found )
           send_to_pager( heading, ch );       /* print report heading */
        found = TRUE;
        
        outer_obj = obj;
        while ( outer_obj->in_obj )
              outer_obj = outer_obj->in_obj;

        SPRINTF(field, "%-18s", obj_short(obj));
        trunc1(field, 18);
        SPRINTF(buf, "%3d &R&w%5d &R&w%-18s &R&w", ++icnt, obj->pIndexData->vnum, field);
        if ( outer_obj->carried_by )
        {
            SPRINTF(field, "%-18s", PERS(outer_obj->carried_by, ch));
            trunc1(field, 18);
            STRAPP(buf, "%5d %-18s &R&w",
               (IS_NPC(outer_obj->carried_by) ?
                outer_obj->carried_by->pIndexData->vnum : 0), field);
            if ( outer_obj!=obj )
            {
               SPRINTF(field, "%-18s", obj->in_obj->name);
               trunc1(field, 18);
               STRAPP(buf, "%5d %-18s &R&w",
               obj->in_obj->pIndexData->vnum, field);
            }
            STRAPP(buf, "&R&w\n");
            send_to_pager(buf, ch);
        }
        else if ( outer_obj->in_room )
        {
           SPRINTF(field, "%-18s", outer_obj->in_room->name);
           trunc1(field, 18);
           STRAPP(buf, "%5d %-18s &R&w",
           outer_obj->in_room->vnum, field);
           if ( outer_obj!=obj )
           {
              SPRINTF(field, "%-18s", obj->in_obj->name);
              trunc1(field, 18);
              STRAPP(buf, "%5d %-18s &R&w",
              obj->in_obj->pIndexData->vnum, field);
           }
           STRAPP(buf, "&R&w\n");
           send_to_pager(buf, ch);
        }
    }
    if ( !found )
      pager_printf(ch, "None found.\n");
}

/*
 * Find the position of a target substring in a source string.
 * Returns pointer to the first occurrence of the string pointed to 
 * bstr in the string pointed to by astr. It returns a null pointer
 * if no match is found.  --  Gorog (with help from Thoric)
 *
 * Note I made a change when modifying str_infix. If the target string is
 * null, I return NULL (meaning no match was found). str_infix returns
 * FALSE (meaning a match was found).  *grumble*
 */
char *str_str( char *astr, char *bstr )
{
    int sstr1, sstr2, ichar;
    char c0;

    if ( ( c0 = LOWER(bstr[0]) ) == '\0' )
        return NULL;

    sstr1 = strlen(astr);
    sstr2 = strlen(bstr);

    for ( ichar = 0; ichar <= sstr1 - sstr2; ichar++ )
        if ( c0 == LOWER(astr[ichar]) && !str_prefix(bstr, astr+ichar) )
            return (astr+ichar);
    return NULL;
}

/*
 * Counts the number of times a target string occurs in a source string.
 * case insensitive -- Gorog
 */
int str_count(char *psource, char *ptarget)
{
   char *ptemp=psource;
   int count=0;
   
   while ( (ptemp=str_str(ptemp, ptarget)) )
   {
      ptemp++;
      count++;
   }
   return count;
}

/*
 * Displays the help screen for the "opfind" command
 */
void opfind_help (CHAR_DATA *ch)
{
   send_to_char( "Syntax:\n", ch);
   send_to_char( "opfind n lo_vnum hi_vnum text \n"
      "   Search obj vnums between lo_vnum and hi_vnum \n"
      "   for obj progs that contain an occurrence of text. \n"
      "   Display a maxiumu of n lines.\n\n", ch );
   send_to_char( "opfind n mud text \n"
      "   Search all the objs in the mud for\n"
      "   obj progs that contain an occurrence of text. \n"
      "   Display a maxiumu of n lines.\n\n", ch );
   
   send_to_char( "Example:\n", ch);
   send_to_char( "opfind 20 901 969 if isnpc \n"
      "   Search all obj progs in Olympus (vnums 901 thru 969)\n"
      "   and display all objects that contain the text \"if isnpc\".\n"
      "   Display a maximum of 20 lines.\n\n", ch );
   send_to_char( "Example:\n", ch);
   send_to_char( "opfind 100 mud mpslay \n"
      "   Search all obj progs in the entire mud\n"
      "   and display all objects that contain the text \"mpslay\".\n"
      "   Display a maximum of 100 lines.\n\n", ch );
}

/*
 * Search objects for obj progs containing a specified text string.
 */
void do_opfind( CHAR_DATA *ch, char *argument )   /* Gorog */
{
   OBJ_INDEX_DATA  *   pObj;
   MPROG_DATA      *   pProg;
   char                arg1 [MAX_INPUT_LENGTH];
   char                arg2 [MAX_INPUT_LENGTH];
   char                arg3 [MAX_INPUT_LENGTH];
   int                 lo_vnum=1, hi_vnum=32767;
   int                 tot_vnum, tot_hits=0;
   int                 i, disp_cou=0, disp_limit;
   
   argument = one_argument( argument, arg1 );   /* display_limit */
   argument = one_argument( argument, arg2 );

   if ( arg1[0]=='\0' || arg2[0]=='\0' || !is_number(arg1) )
   {
      opfind_help(ch);
      return;
   }

   disp_limit = atoi (arg1);
   disp_limit = UMAX(0, disp_limit);

   if ( str_cmp(arg2, "mud") )
   {
      argument = one_argument( argument, arg3 );
      if ( arg3[0]=='\0' || argument[0]=='\0' 
      ||   !is_number(arg2) || !is_number(arg3) )
      {
         opfind_help(ch);
         return;
      }
      else
      {
         lo_vnum = URANGE(1, atoi(arg2), 32767);
         hi_vnum = URANGE(1, atoi(arg3), 32767);
         if ( lo_vnum > hi_vnum )
         {
            opfind_help(ch);
            return;
         }
      }
   }   
   if ( argument[0] == '\0' )
   {
      opfind_help(ch);
      return;
   }

   for (i = lo_vnum; i <= hi_vnum; i++)
   {
      if ( (pObj=get_obj_index(i)) && (pProg=pObj->mudprogs) )
      {
         tot_vnum = 0;
         for ( ; pProg; pProg=pProg->next)
             tot_vnum += str_count(pProg->comlist, argument);
         tot_hits += tot_vnum;
         if ( tot_vnum && ++disp_cou <= disp_limit)
            pager_printf( ch, "%5d %5d %5d\n", disp_cou, i, tot_vnum);
      }
   }
   pager_printf( ch, "Total: %10d\n", tot_hits);
}
		
/*
 * Displays the help screen for the "mpfind" command
 */
void mpfind_help (CHAR_DATA *ch)
{
   send_to_char( "Syntax:\n", ch);
   send_to_char( "mpfind n lo_vnum hi_vnum text \n"
      "   Search mob vnums between lo_vnum and hi_vnum \n"
      "   for mob progs that contain an occurrence of text. \n"
      "   Display a maxiumu of n lines.\n\n", ch );
   send_to_char( "mpfind n mud text \n"
      "   Search all the mobs in the mud for\n"
      "   mob progs that contain an occurrence of text. \n"
      "   Display a maxiumu of n lines.\n\n", ch );
   
   send_to_char( "Example:\n", ch);
   send_to_char( "mpfind 20 901 969 if isnpc \n"
      "   Search all mob progs in Olympus (vnums 901 thru 969)\n"
      "   and display all mobs that contain the text \"if isnpc\".\n"
      "   Display a maximum of 20 lines.\n\n", ch );
   send_to_char( "Example:\n", ch);
   send_to_char( "mpfind 100 mud mpslay \n"
      "   Search all mob progs in the entire mud\n"
      "   and display all mobs that contain the text \"mpslay\".\n"
      "   Display a maximum of 100 lines.\n\n", ch );
}

/*
 * Search mobs for mob progs containing a specified text string.
 */
void do_mpfind( CHAR_DATA *ch, char *argument )   /* Gorog */
{
   MOB_INDEX_DATA  *   pMob;
   MPROG_DATA      *   pProg;
   char                arg1 [MAX_INPUT_LENGTH];
   char                arg2 [MAX_INPUT_LENGTH];
   char                arg3 [MAX_INPUT_LENGTH];
   int                 lo_vnum=1, hi_vnum=32767;
   int                 tot_vnum, tot_hits=0;
   int                 i, disp_cou=0, disp_limit;
   
   argument = one_argument( argument, arg1 );   /* display_limit */
   argument = one_argument( argument, arg2 );

   if ( arg1[0]=='\0' || arg2[0]=='\0' || !is_number(arg1) )
   {
      mpfind_help(ch);
      return;
   }

   disp_limit = atoi (arg1);
   disp_limit = UMAX(0, disp_limit);

   if ( str_cmp(arg2, "mud") )
   {
      argument = one_argument( argument, arg3 );
      if ( arg3[0]=='\0' || !is_number(arg2) || !is_number(arg3) )
      {
         mpfind_help(ch);
         return;
      }
      else
      {
         lo_vnum = URANGE(1, atoi(arg2), 32767);
         hi_vnum = URANGE(1, atoi(arg3), 32767);
         if ( lo_vnum > hi_vnum )
         {
            mpfind_help(ch);
            return;
         }
      }
   }   
   if ( argument[0] == '\0' )
   {
      mpfind_help(ch);
      return;
   }

   for (i = lo_vnum; i <= hi_vnum; i++)
   {
      if ( (pMob=get_mob_index(i)) && (pProg=pMob->mudprogs) )
      {
         tot_vnum = 0;
         for ( ; pProg; pProg=pProg->next)
             tot_vnum += str_count(pProg->comlist, argument);
         tot_hits += tot_vnum;
         if ( tot_vnum && ++disp_cou <= disp_limit)
            pager_printf( ch, "%5d %5d %5d\n", disp_cou, i, tot_vnum);
      }
   }
   pager_printf( ch, "Total: %10d\n", tot_hits);
}
		
/*
 * Displays the help screen for the "rpfind" command
 */
void rpfind_help (CHAR_DATA *ch)
{
   send_to_char( "Syntax:\n", ch);
   send_to_char( "rpfind n lo_vnum hi_vnum text \n"
      "   Search room vnums between lo_vnum and hi_vnum \n"
      "   for room progs that contain an occurrence of text. \n"
      "   Display a maxiumu of n lines.\n\n", ch );
   send_to_char( "rpfind n mud text \n"
      "   Search all the rooms in the mud for\n"
      "   room progs that contain an occurrence of text. \n"
      "   Display a maxiumu of n lines.\n\n", ch );
   
   send_to_char( "Example:\n", ch);
   send_to_char( "rpfind 20 901 969 if isnpc \n"
      "   Search all room progs in Olympus (vnums 901 thru 969)\n"
      "   and display all vnums that contain the text \"if isnpc\".\n"
      "   Display a maximum of 20 lines.\n\n", ch );
   send_to_char( "Example:\n", ch);
   send_to_char( "rpfind 100 mud mpslay \n"
      "   Search all room progs in the entire mud\n"
      "   and display all vnums that contain the text \"mpslay\".\n"
      "   Display a maximum of 100 lines.\n\n", ch );
}

/*
 * Search rooms for room progs containing a specified text string.
 */
void do_rpfind( CHAR_DATA *ch, char *argument )   /* Gorog */
{
   ROOM_INDEX_DATA *   pRoom;
   MPROG_DATA      *   pProg;
   char                arg1 [MAX_INPUT_LENGTH];
   char                arg2 [MAX_INPUT_LENGTH];
   char                arg3 [MAX_INPUT_LENGTH];
   int                 lo_vnum=1, hi_vnum=32767;
   int                 tot_vnum, tot_hits=0;
   int                 i, disp_cou=0, disp_limit;
   
   argument = one_argument( argument, arg1 );   /* display_limit */
   argument = one_argument( argument, arg2 );

   if ( arg1[0]=='\0' || arg2[0]=='\0' || !is_number(arg1) )
   {
      rpfind_help(ch);
      return;
   }

   disp_limit = atoi (arg1);
   disp_limit = UMAX(0, disp_limit);

   if ( str_cmp(arg2, "mud") )
   {
      argument = one_argument( argument, arg3 );
      if ( arg3[0]=='\0' || argument[0]=='\0' 
      ||   !is_number(arg2) || !is_number(arg3) )
      {
         rpfind_help(ch);
         return;
      }
      else
      {
         lo_vnum = URANGE(1, atoi(arg2), 32767);
         hi_vnum = URANGE(1, atoi(arg3), 32767);
         if ( lo_vnum > hi_vnum )
         {
            rpfind_help(ch);
            return;
         }
      }
   }   
   if ( argument[0] == '\0' )
   {
      rpfind_help(ch);
      return;
   }
/*
   pager_printf( ch, "display:%d lo:%d hi:%d test=\"%s\"\n", 
                 disp_limit, lo_vnum, hi_vnum, argument);
*/
   for (i = lo_vnum; i <= hi_vnum; i++)
   {
      if ( (pRoom=get_room_index(i)) && (pProg=pRoom->mudprogs) )
      {
         tot_vnum = 0;
         for ( ; pProg; pProg=pProg->next)
             tot_vnum += str_count(pProg->comlist, argument);
         tot_hits += tot_vnum;
         if ( tot_vnum && ++disp_cou <= disp_limit)
            pager_printf( ch, "%5d %5d %5d\n", disp_cou, i, tot_vnum);
      }
   }
   pager_printf( ch, "Total: %10d\n", tot_hits);
}

void do_reboo( CHAR_DATA *ch, char *argument )
{
    send_to_char( "If you want to REBOOT, spell it out.\n", ch );
    return;
}



void do_reboot( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    extern bool mud_down;
    CHAR_DATA *vch;
    SHIP_DATA *ship;

    if ( str_cmp( argument, "mud now" )
    &&   str_cmp( argument, "nosave" )
    &&   str_cmp( argument, "and sort skill table" ) )
    {
	send_to_char( "Syntax: 'reboot mud now' or 'reboot nosave'\n", ch );
	return;
    }

    if ( auction->item )
	do_auction( ch, "stop");

    SPRINTF( buf, "Reboot by %s.", ch->name );
    do_echo( ch, buf );

    if ( !str_cmp(argument, "and sort skill table") )
    {
	sort_skill_table();
	save_skill_table();
    }

    /* Save all characters before booting. */
    if ( str_cmp(argument, "nosave") )
	for ( vch = first_char; vch; vch = vch->next )
	    if ( !IS_NPC( vch ) )
		save_char_obj( vch );

    for ( ship = first_ship; ship; ship = ship->next )
      save_ship( ship );

    mud_down = TRUE;
    return;
}



void do_shutdow( CHAR_DATA *ch, char *argument )
{
    send_to_char( "If you want to SHUTDOWN, spell it out.\n", ch );
    return;
}



void do_shutdown( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    extern bool mud_down;
    CHAR_DATA *vch;
    SHIP_DATA *ship;

    if ( str_cmp( argument, "mud now" ) && str_cmp(argument, "nosave") )
    {
	send_to_char( "Syntax: 'shutdown mud now' or 'shutdown nosave'\n", ch );
	return;
    }

    if ( auction->item )
	do_auction( ch, "stop");

    SPRINTF( buf, "Shutdown by %s.", ch->name );
    append_file( ch, SHUTDOWN_FILE, buf );
    STRAPP( buf, "\n" );
    do_echo( ch, buf );

    /* Save all characters before booting. */
    if ( str_cmp(argument, "nosave") )
    {
	for ( vch = first_char; vch; vch = vch->next )
	    if ( !IS_NPC( vch ) )
		save_char_obj( vch );
        for ( ship = first_ship; ship; ship = ship->next )
            save_ship( ship );
    }
		
    mud_down = TRUE;
    return;
}


void do_snoop( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    DESCRIPTOR_DATA *d;
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Snoop whom?\n", ch );
	return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n", ch );
	return;
    }

    if ( !victim->desc )
    {
	send_to_char( "No descriptor to snoop.\n", ch );
	return;
    }

    if ( victim == ch )
    {
	send_to_char( "Cancelling all snoops.\n", ch );
	for ( d = first_descriptor; d; d = d->next )
	    if ( d->snoop_by == ch->desc )
		d->snoop_by = NULL;
	return;
    }

    if ( victim->desc->snoop_by )
    {
	send_to_char( "Busy already.\n", ch );
	return;
    }

    /*
     * Minimum snoop level... a secret mset value
     * makes the snooper think that the victim is already being snooped
     */
    if ( get_trust( victim ) >= get_trust( ch )
    ||  (victim->pcdata && victim->pcdata->min_snoop > get_trust( ch )) )
    {
	send_to_char( "Busy already.\n", ch );
	return;
    }

    if ( ch->desc )
    {
	for ( d = ch->desc->snoop_by; d; d = d->snoop_by )
	    if ( d->character == victim || d->original == victim )
	    {
		send_to_char( "No snoop loops.\n", ch );
		return;
	    }
    }

/*  Snoop notification for higher imms, if desired, uncomment this
    if ( get_trust(victim) > LEVEL_GOD && get_trust(ch) < LEVEL_SUPREME )
      send_to_char( victim, "\nYou feel like someone is watching your every move...\n", 0 );
*/
    victim->desc->snoop_by = ch->desc;
    send_to_char( "Ok.\n", ch );
    return;
}



void do_switch( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Switch into whom?\n", ch );
	return;
    }

    if ( !ch->desc )
	return;

    if ( ch->desc->original )
    {
	send_to_char( "You are already switched.\n", ch );
	return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n", ch );
	return;
    }

    if ( victim == ch )
    {
	send_to_char( "Ok.\n", ch );
	return;
    }

    if ( victim->desc )
    {
	send_to_char( "Character in use.\n", ch );
	return;
    }

    if ( !IS_NPC(victim) && get_trust(ch) < LEVEL_GREATER )
    {
	send_to_char( "You cannot switch into a player!\n", ch );
	return;
    }

    ch->desc->character = victim;
    ch->desc->original  = ch;
    victim->desc        = ch->desc;
    ch->desc            = NULL;
    ch->switched	= victim;
    send_to_char( "Ok.\n", victim );
    return;
}



void do_return( CHAR_DATA *ch, char *argument )
{
    if ( !ch->desc )
	return;

    if ( !ch->desc->original )
    {
	send_to_char( "You aren't switched.\n", ch );
	return;
    }

    if (BV_IS_SET(ch->act, ACT_POLYMORPHED))
    {
      send_to_char("Use revert to return from a polymorphed mob.\n", ch);
      return;
    }

    send_to_char( "You return to your original body.\n", ch );
	if ( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_POSSESS ) )
	{
		affect_strip( ch, gsn_possess );
		BV_REMOVE_BIT( ch->affected_by, AFF_POSSESS );
	}
/*    if ( IS_NPC( ch->desc->character ) )
      BV_REMOVE_BIT( ch->desc->character->affected_by, AFF_POSSESS );*/
    ch->desc->character       = ch->desc->original;
    ch->desc->original        = NULL;
    ch->desc->character->desc = ch->desc;
    ch->desc->character->switched = NULL;
    ch->desc                  = NULL;
    return;
}



void do_minvoke( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    MOB_INDEX_DATA *pMobIndex;
    CHAR_DATA *victim;
    sh_int vnum;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Syntax: minvoke <vnum>.\n", ch );
	return;
    }

    if ( !is_number( arg ) )
    {
	char arg2[MAX_INPUT_LENGTH];
	int  hash, cnt;
	int  count = number_argument( arg, arg2 );

	vnum = -1;
	for ( hash = cnt = 0; hash < MAX_KEY_HASH; hash++ )
	    for ( pMobIndex = mob_index_hash[hash];
		  pMobIndex;
		  pMobIndex = pMobIndex->next )
	    if ( nifty_is_name( arg2, pMobIndex->player_name )
	    &&   ++cnt == count )
	    {
		vnum = pMobIndex->vnum;
		break;
	    }
	if ( vnum == -1 )
	{
	    send_to_char( "No such mobile exists.\n", ch );
	    return;
	}
    }
    else
	vnum = atoi( arg );

    if ( get_trust(ch) < LEVEL_DEMI )
    {
	AREA_DATA *pArea;

	if ( IS_NPC(ch) )
	{
	  send_to_char( "Huh?\n", ch );
	  return;
	}

	if ( !ch->pcdata || !(pArea=ch->pcdata->area) )
	{
	  send_to_char( "You must have an assigned area to invoke this mobile.\n", ch );
	  return;
	}
	if ( vnum < pArea->low_m_vnum
	||   vnum > pArea->hi_m_vnum )
	{
	  send_to_char( "That number is not in your allocated range.\n", ch );
	  return;
	}
    }

    if ( ( pMobIndex = get_mob_index( vnum ) ) == NULL )
    {
	send_to_char( "No mobile has that vnum.\n", ch );
	return;
    }

    victim = create_mobile( pMobIndex );
    char_to_room( victim, ch->in_room );
    act( AT_IMMORT, "$n has created $N!", ch, NULL, victim, TO_ROOM );
    send_to_char( "Ok.\n", ch );
    return;
}



void do_oinvoke( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    OBJ_INDEX_DATA *pObjIndex;
    OBJ_DATA *obj;
    sh_int vnum;
    int level;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' )
    {
	send_to_char( "Syntax: oinvoke <vnum> <level>.\n", ch );
	return;
    }

    if ( arg2[0] == '\0' )
    {
	level = get_trust( ch );
    }
    else
    {
	if ( !is_number( arg2 ) )
	{
	    send_to_char( "Syntax: oinvoke <vnum> <level>.\n", ch );
	    return;
	}
	level = atoi( arg2 );
	if ( level < 0 || level > get_trust( ch ) )
	{
	    send_to_char( "Limited to your trust level.\n", ch );
	    return;
        }
    }

    if ( !is_number( arg1 ) )
    {
	char arg[MAX_INPUT_LENGTH];
	int  hash, cnt;
	int  count = number_argument( arg1, arg );

	vnum = -1;
	for ( hash = cnt = 0; hash < MAX_KEY_HASH; hash++ )
	    for ( pObjIndex = obj_index_hash[hash];
		  pObjIndex;
		  pObjIndex = pObjIndex->next )
	    if ( nifty_is_name( arg, pObjIndex->name )
	    &&   ++cnt == count )
	    {
		vnum = pObjIndex->vnum;
		break;
	    }
	if ( vnum == -1 )
	{
	    send_to_char( "No such object exists.\n", ch );
	    return;
	}
    }
    else
	vnum = atoi( arg1 );

    if ( get_trust(ch) < LEVEL_DEMI )
    {
	AREA_DATA *pArea;

	if ( IS_NPC(ch) )
	{
	  send_to_char( "Huh?\n", ch );
	  return;
	}
	
	if ( !ch->pcdata || !(pArea=ch->pcdata->area) )
	{
	  send_to_char( "You must have an assigned area to invoke this object.\n", ch );
	  return;
	}
	if ( vnum < pArea->low_o_vnum
	||   vnum > pArea->hi_o_vnum )
	{
	  send_to_char( "That number is not in your allocated range.\n", ch );
	  return;
	}
    }

    if ( ( pObjIndex = get_obj_index( vnum ) ) == NULL )
    {
	send_to_char( "No object has that vnum.\n", ch );
	return;
    }

/* Commented out by Narn, it seems outdated
    if ( IS_OBJ_STAT( pObjIndex, ITEM_PROTOTYPE )
    &&	 pObjIndex->count > 5 )
    {
	send_to_char( "That object is at its limit.\n", ch );
	return;
    }
*/

    obj = create_object( pObjIndex, level );
    if ( CAN_WEAR(obj, ITEM_TAKE) )
    {
	obj = obj_to_char( obj, ch );
    }
    else
    {
	obj = obj_to_room( obj, ch->in_room );
	act( AT_IMMORT, "$n has created $p!", ch, obj, NULL, TO_ROOM );
    }
    send_to_char( "Ok.\n", ch );
    return;
}



void do_purge( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *obj;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	/* 'purge' */
	CHAR_DATA *vnext;
	OBJ_DATA  *obj_next;

	for ( victim = ch->in_room->first_person; victim; victim = vnext )
	{
	    vnext = victim->next_in_room;
	    if ( IS_NPC(victim) && victim != ch && !BV_IS_SET(victim->act, ACT_POLYMORPHED))
		extract_char( victim, TRUE );
	}

	for ( obj = ch->in_room->first_content; obj; obj = obj_next )
	{
	    obj_next = obj->next_content;
	    if ( obj->item_type == ITEM_SPACECRAFT )
	        continue;
	    extract_obj( obj );
	}

	act( AT_IMMORT, "$n purges the room!", ch, NULL, NULL, TO_ROOM);
	send_to_char( "Ok.\n", ch );
	return;
    }
    victim = NULL; obj = NULL;

    /* fixed to get things in room first -- i.e., purge portal (obj),
     * no more purging mobs with that keyword in another room first
     * -- Tri */
    if ( ( victim = get_char_room( ch, arg ) ) == NULL 
    && ( obj = get_obj_here( ch, arg ) ) == NULL )
    {
      if ( ( victim = get_char_world( ch, arg ) ) == NULL
      &&   ( obj = get_obj_world( ch, arg ) ) == NULL )  /* no get_obj_room */
      {
	send_to_char( "They aren't here.\n", ch );
	return;
      }
    }

/* Single object purge in room for high level purge - Scryn 8/12*/
    if ( obj )
    {
	separate_obj( obj );
	act( AT_IMMORT, "$n purges $p.", ch, obj, NULL, TO_ROOM);
	act( AT_IMMORT, "You make $p disappear in a puff of smoke!", ch, obj, NULL, TO_CHAR);          
	extract_obj( obj );
	return;
    }


    if ( !IS_NPC(victim) )
    {
	send_to_char( "Not on PC's.\n", ch );
	return;
    }

    if ( victim == ch )
    {
    	send_to_char( "You cannot purge yourself!\n", ch );
    	return;
    }

    if (BV_IS_SET(victim->act, ACT_POLYMORPHED))
    {
      send_to_char("You cannot purge a polymorphed player.\n", ch);
      return;
    }
    act( AT_IMMORT, "$n purges $N.", ch, NULL, victim, TO_NOTVICT );
    extract_char( victim, TRUE );
    return;
}


void do_low_purge( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *obj;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Purge what?\n", ch );
	return;
    }

    victim = NULL; obj = NULL;
    if ( ( victim = get_char_room( ch, arg ) ) == NULL
    &&	 ( obj    = get_obj_here ( ch, arg ) ) == NULL )
    {
	send_to_char( "You can't find that here.\n", ch );
	return;
    }

    if ( obj )
    {
	separate_obj( obj );
	act( AT_IMMORT, "$n purges $p!", ch, obj, NULL, TO_ROOM );
	act( AT_IMMORT, "You make $p disappear in a puff of smoke!", ch, obj, NULL, TO_CHAR );
	extract_obj( obj );
	return;
    }

    if ( !IS_NPC(victim) )
    {
	send_to_char( "Not on PC's.\n", ch );
	return;
    }

    if ( victim == ch )
    {
    	send_to_char( "You cannot purge yourself!\n", ch );
    	return;
    }

    act( AT_IMMORT, "$n purges $N.", ch, NULL, victim, TO_NOTVICT );
    act( AT_IMMORT, "You make $N disappear in a puff of smoke!", ch, NULL, victim, TO_CHAR );
    extract_char( victim, TRUE );
    return;
}


void do_balzhur( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    int sn;

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Who is deserving of such a fate?\n", ch );
	return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't playing.\n", ch);
	return;
    }

    if ( IS_NPC(victim) )
    {
	send_to_char( "Not on NPC's.\n", ch );
	return;
    }

    if ( get_trust( victim ) >= get_trust( ch ) )
    {
	send_to_char( "I wouldn't even think of that if I were you...\n", ch );
	return;
    }

    victim->top_level = 1;
    victim->trust = 0;
    check_switch( victim, TRUE );
    set_char_color( AT_WHITE, ch );
    send_to_char( "You summon the demon Balzhur to wreak your wrath!\n", ch );
    send_to_char( "Balzhur sneers at you evilly, then vanishes in a puff of smoke.\n", ch );
    set_char_color( AT_IMMORT, victim );
    send_to_char( "You hear an ungodly sound in the distance that makes your blood run cold!\n", victim );
    snprintf( buf, MAX_STRING_LENGTH, "Balzhur screams, 'You are MINE %s!!!'", victim->name );
    echo_to_all( AT_IMMORT, buf, ECHOTAR_ALL );
	{
	   for ( int ability = 0 ; ability < MAX_ABILITY ; ability++ )
	   {
	       victim->experience[ability] = 1;
	       victim->skill_level[ability] = 1;
	   }
	}
	victim->max_hit  = 500;
	victim->max_mana = 0;
	victim->max_move = 1000;
	for ( sn = 0; sn < top_sn; sn++ )
	    victim->pcdata->learned[sn] = 0;
	victim->hit      = victim->max_hit;
	victim->mana     = victim->max_mana;
	victim->move     = victim->max_move;
    gmcp_evt_char_vitals(victim);

    SPRINTF( buf, "%s%s", GOD_DIR, capitalize(victim->name) );  
 
    if ( !remove( buf ) )
      send_to_char( "Player's immortal data destroyed.\n", ch );
    else if ( errno != ENOENT )
    {
      ch_printf( ch, "Unknown error #%d - %s (immortal data).  Report to the admins.\n",
              errno, strerror( errno ) );
      SPRINTF( buf2, "%s balzhuring %s", ch->name, buf );
      perror( buf2 );
    }
    SPRINTF( buf2, "%s.are", capitalize(arg) );
/*    for ( pArea = first_build; pArea; pArea = pArea->next )
      if ( pArea && !str_cmp( pArea->filename, buf2 ) )
      {
        SPRINTF( buf, "%s%s", BUILD_DIR, buf2 );
        if ( IS_SET( pArea->status, AREA_LOADED ) )
          fold_area( pArea, buf, FALSE );
        close_area( pArea );
        SPRINTF( buf2, "%s.bak", buf );
        set_char_color( AT_RED, ch );  Log message changes colors 
        if ( !rename( buf, buf2 ) )
          send_to_char( "Player's area data destroyed.  Area saved as 
backup.\n", ch);
        else if ( errno != ENOENT )
        {
          ch_printf( ch, "Unknown error #%d - %s (area data).  Report to 
Thoric.\n",
                  errno, strerror( errno ) );
          SPRINTF( buf2, "%s destroying %s", ch->name, buf );
          perror( buf2 );   
        }
      }
 */

 
        make_wizlist();
	do_help(victim, "M_BALZHUR_" );
	set_char_color( AT_WHITE, victim );
	send_to_char( "You awake after a long period of time...\n", victim );
	while ( victim->first_carrying )
	     extract_obj( victim->first_carrying );
    return;
}

void do_advance( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int level, ability;
    int iLevel, iAbility;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg3 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' || !is_number( arg2 ) )
    {
	send_to_char( "Syntax: advance <char> <ability> <level>.\n", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg1 ) ) == NULL )
    {
	send_to_char( "That player is not here.\n", ch);
	return;
    }

    ability = -1;
    for ( iAbility = 0 ; iAbility < MAX_ABILITY ; iAbility++ )
    {
       if ( !str_prefix( arg3 , ability_name[iAbility] ) )
       {
         ability = iAbility;
         break;
       }
    }
    
    if ( ability == -1 )
    {
	send_to_char( "No Such Ability.\n", ch);
	do_advance(ch, "" );
	return;
    }

    
    if ( IS_NPC(victim) )
    {
	send_to_char( "Not on NPC's.\n", ch );
	return;
    }

    /* You can demote yourself but not someone else at your own trust. -- Narn */
    if ( get_trust( ch ) <= get_trust( victim ) && ch != victim )
    {
	send_to_char( "You can't do that.\n", ch );
	return;
    }

    if ( ( level = atoi( arg2 ) ) < 1 || level > 500 )
    {
	send_to_char( "Level must be 1 to 500.\n", ch );
	return;
    }

    /*
     * Lower level:
     *   Reset to level 1.
     *   Then raise again.
     *   Currently, an imp can lower another imp.
     *   -- Swiftest
     */
    if ( level <= victim->skill_level[ability] )
    {
	send_to_char( "Lowering a player's level!\n", ch );
	set_char_color( AT_IMMORT, victim );
	send_to_char( "Cursed and forsaken! The gods have lowered your level.\n", victim );
        victim->experience[ability] = 0;
        victim->skill_level[ability] = 1;
        if ( ability == COMBAT_ABILITY )
          victim->max_hit = 500;
        if ( ability == FORCE_ABILITY )
          victim->max_mana = 0;
       gmcp_evt_char_vitals(ch);
    }
    else
    {
	send_to_char( "Raising a player's level!\n", ch );
	  send_to_char( "The gods feel fit to raise your level!\n", victim );
    }

    for ( iLevel = victim->skill_level[ability] ; iLevel < level; iLevel++ )
    {
	victim->experience[ability] = exp_level(iLevel+1);
	gain_exp( victim, 0 , ability );
    }
    return;
}

void do_immortalize( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Syntax: immortalize <char>\n", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "That player is not here.\n", ch);
	return;
    }

    if ( IS_NPC(victim) )
    {
	send_to_char( "Not on NPC's.\n", ch );
	return;
    }

    if ( victim->top_level != LEVEL_AVATAR )
    {
	send_to_char( "This player is not worthy of immortality yet.\n", ch );
	return;
    }

    send_to_char( "Immortalizing a player...\n", ch );
    set_char_color( AT_IMMORT, victim );
    act( AT_IMMORT, "$n begins to chant softly... then raises $s arms to the sky...",
	 ch, NULL, NULL, TO_ROOM );
    set_char_color( AT_WHITE, victim );
    send_to_char( "You suddenly feel very strange...\n\n", victim );
    set_char_color( AT_LBLUE, victim );

    do_help(victim, "M_GODLVL1_" );
    set_char_color( AT_WHITE, victim );
    send_to_char( "You awake... all your possessions are gone.\n", victim );
    while ( victim->first_carrying )
	extract_obj( victim->first_carrying );

    victim->top_level = LEVEL_IMMORTAL;
   
/*    advance_level( victim );  */

    victim->trust = 0;
    return;
}



void do_trust( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int level;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' || !is_number( arg2 ) )
    {
	send_to_char( "Syntax: trust <char> <level>.\n", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg1 ) ) == NULL )
    {
	send_to_char( "That player is not here.\n", ch);
	return;
    }

    if ( ( level = atoi( arg2 ) ) < 0 || level > MAX_LEVEL )
    {
	send_to_char( "Level must be 0 (reset) or 1 to 60.\n", ch );
	return;
    }

    if ( level > get_trust( ch ) )
    {
	send_to_char( "Limited to your own trust.\n", ch );
	return;
    }

    if ( ch->top_level < LEVEL_SUPREME && get_trust( victim ) >= get_trust( ch ) )
    {
	send_to_char( "You can't do that.\n", ch );
	return;
    }

    victim->trust = level;
    send_to_char( "Ok.\n", ch );
    return;
}

void do_toplevel( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int level;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' || !is_number( arg2 ) )
    {
	send_to_char( "Syntax: toplevel <char> <level>.\n", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg1 ) ) == NULL )
    {
	send_to_char( "That player is not here.\n", ch);
	return;
    }

    if ( ( level = atoi( arg2 ) ) < 0 || level > MAX_LEVEL )
    {
	send_to_char( "Level must be 0 (reset) or 1 to 60.\n", ch );
	return;
    }

    if ( level > ch->top_level )
    {
	send_to_char( "Limited to your own top level.\n", ch );
	return;
    }

    if ( ch->top_level < LEVEL_SUPREME && victim->top_level >= ch->top_level )
    {
	send_to_char( "You can't do that.\n", ch );
	return;
    }

    victim->top_level = level;
    send_to_char( "Ok.\n", ch );
    return;
}


void do_restore( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
	send_to_char( "Restore whom?\n", ch );
	return;
    }

    if ( !str_cmp( arg, "all" ) )
    {
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;

        if ( !ch->pcdata )
          return;

        if ( get_trust( ch ) < LEVEL_SUB_IMPLEM )
        {
          if ( IS_NPC( ch ) )
          {
  	    send_to_char( "You can't do that.\n", ch );
 	    return;
          }
          else
          {
            /* Check if the player did a restore all within the last 18 hours. */
            if ( current_time - ch->pcdata->restore_time < RESTORE_INTERVAL ) 
            {
              send_to_char( "Sorry, you can't do a restore all yet.\n", ch ); 
              do_restoretime( ch, "" );
              return;
            }
          }
        }
        last_restore_all_time    = current_time;
        ch->pcdata->restore_time = current_time;
        save_char_obj( ch );
        send_to_char( "Ok.\n", ch);
	for ( vch = first_char; vch; vch = vch_next )
	{
	    vch_next = vch->next;

	    if ( !IS_NPC( vch ) && !IS_IMMORTAL( vch ) )
	    {
   		vch->hit = vch->max_hit;
		vch->mana = vch->max_mana;
		vch->move = vch->max_move;
        gmcp_evt_char_vitals(vch);
		vch->pcdata->condition[COND_BLOODTHIRST] = (10 + vch->top_level);
		update_pos (vch);
		act( AT_IMMORT, "$n has restored you.", ch, NULL, vch, TO_VICT);
	    }
	}
    }
    else
    {    

    CHAR_DATA *victim;

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n", ch );
	return;
    }

    if ( get_trust( ch ) < LEVEL_LESSER 
      &&  victim != ch
      && !( IS_NPC( victim ) && BV_IS_SET( victim->act, ACT_PROTOTYPE ) ) )
    { 
      send_to_char( "You can't do that.\n", ch );
      return;
    }

    victim->hit  = victim->max_hit;
    victim->mana = victim->max_mana;
    victim->move = victim->max_move;
    gmcp_evt_char_vitals(victim);
    if ( victim->pcdata )
      victim->pcdata->condition[COND_BLOODTHIRST] = (10 + victim->top_level);
    update_pos( victim );
    if ( ch != victim )
      act( AT_IMMORT, "$n has restored you.", ch, NULL, victim, TO_VICT );
    send_to_char( "Ok.\n", ch );
    return;
    }
}

void do_restoretime( CHAR_DATA *ch, char *argument )
{
  long int time_passed;
  int hour, minute;

  if ( !last_restore_all_time )
     ch_printf( ch, "There has been no restore all since reboot\n");
  else
     {
     time_passed = current_time - last_restore_all_time;
     hour = (int) ( time_passed / 3600 );
     minute = (int) ( ( time_passed - ( hour * 3600 ) ) / 60 );
     ch_printf( ch, "The  last restore all was %d hours and %d minutes ago.\n", 
                  hour, minute );
     }

  if ( !ch->pcdata )
    return;

  if ( !ch->pcdata->restore_time )
  {
    send_to_char( "You have never done a restore all.\n", ch );
    return;
  }

  time_passed = current_time - ch->pcdata->restore_time;
  hour = (int) ( time_passed / 3600 );
  minute = (int) ( ( time_passed - ( hour * 3600 ) ) / 60 );
  ch_printf( ch, "Your last restore all was %d hours and %d minutes ago.\n", 
                  hour, minute ); 
  return;
}

void do_freeze( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Freeze whom?\n", ch );
	return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n", ch );
	return;
    }

    if ( IS_NPC(victim) )
    {
	send_to_char( "Not on NPC's.\n", ch );
	return;
    }

   if( get_trust( victim ) >= get_trust( ch ) )
   {
      send_to_char( "You failed.\n", ch );
      return;
   }

   if( victim->desc && victim->desc->original && get_trust(victim->desc->original) >= get_trust(ch) )
   {
      send_to_char( "For some inexplicable reason, you failed.\n", ch );
      return;
   }

   if( BV_IS_SET( victim->act, PLR_FREEZE ) )
   {
      BV_REMOVE_BIT( victim->act, PLR_FREEZE );
      send_to_char( "Your frozen form suddenly thaws.\n", victim );
      ch_printf( ch, "%s is now unfrozen.\n", victim->name );
   }
   else
   {
      if( victim->switched )
      {
         do_return( victim->switched, "" );
         set_char_color( AT_LBLUE, victim );
      }
      BV_SET_BIT( victim->act, PLR_FREEZE );
      send_to_char( "A godly force turns your body to ice!\n", victim );
      ch_printf( ch, "You have frozen %s.\n", victim->name );
   }

    save_char_obj( victim );

    return;
}



void do_log( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Log whom?\n", ch );
	return;
    }

    if ( !str_cmp( arg, "all" ) )
    {
	if ( fLogAll )
	{
	    fLogAll = FALSE;
	    send_to_char( "Log ALL off.\n", ch );
	}
	else
	{
	    fLogAll = TRUE;
	    send_to_char( "Log ALL on.\n", ch );
	}
	return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n", ch );
	return;
    }

    if ( IS_NPC(victim) )
    {
	send_to_char( "Not on NPC's.\n", ch );
	return;
    }

    /*
     * No level check, gods can log anyone.
     */
    if ( BV_IS_SET(victim->act, PLR_LOG) )
    {
	BV_REMOVE_BIT(victim->act, PLR_LOG);
	send_to_char( "LOG removed.\n", ch );
    }
    else
    {
	BV_SET_BIT(victim->act, PLR_LOG);
	send_to_char( "LOG set.\n", ch );
    }

    return;
}


void do_litterbug( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Set litterbug flag on whom?\n", ch );
	return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n", ch );
	return;
    }

    if ( IS_NPC(victim) )
    {
	send_to_char( "Not on NPC's.\n", ch );
	return;
    }

    if ( get_trust( victim ) >= get_trust( ch ) )
    {
	send_to_char( "You failed.\n", ch );
	return;
    }

    if ( BV_IS_SET(victim->act, PLR_LITTERBUG) )
    {
	BV_REMOVE_BIT(victim->act, PLR_LITTERBUG);
	send_to_char( "You can drop items again.\n", victim );
	send_to_char( "LITTERBUG removed.\n", ch );
    }
    else
    {
	BV_SET_BIT(victim->act, PLR_LITTERBUG);
	send_to_char( "You a strange force prevents you from dropping any more items!\n", victim );
	send_to_char( "LITTERBUG set.\n", ch );
    }

    return;
}


void do_noemote( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Noemote whom?\n", ch );
	return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n", ch );
	return;
    }

    if ( IS_NPC(victim) )
    {
	send_to_char( "Not on NPC's.\n", ch );
	return;
    }

    if ( get_trust( victim ) >= get_trust( ch ) )
    {
	send_to_char( "You failed.\n", ch );
	return;
    }

    if ( BV_IS_SET(victim->act, PLR_NO_EMOTE) )
    {
	BV_REMOVE_BIT(victim->act, PLR_NO_EMOTE);
	send_to_char( "You can emote again.\n", victim );
	send_to_char( "NO_EMOTE removed.\n", ch );
    }
    else
    {
	BV_SET_BIT(victim->act, PLR_NO_EMOTE);
	send_to_char( "You can't emote!\n", victim );
	send_to_char( "NO_EMOTE set.\n", ch );
    }

    return;
}



void do_notell( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Notell whom?", ch );
	return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n", ch );
	return;
    }

    if ( IS_NPC(victim) )
    {
	send_to_char( "Not on NPC's.\n", ch );
	return;
    }

    if ( get_trust( victim ) >= get_trust( ch ) )
    {
	send_to_char( "You failed.\n", ch );
	return;
    }

    if ( BV_IS_SET(victim->act, PLR_NO_TELL) )
    {
	BV_REMOVE_BIT(victim->act, PLR_NO_TELL);
	send_to_char( "You can tell again.\n", victim );
	send_to_char( "NO_TELL removed.\n", ch );
    }
    else
    {
	BV_SET_BIT(victim->act, PLR_NO_TELL);
	send_to_char( "You can't tell!\n", victim );
	send_to_char( "NO_TELL set.\n", ch );
    }

    return;
}


void do_notitle( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
 
    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        send_to_char( "Notitle whom?\n", ch );
        return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
        send_to_char( "They aren't here.\n", ch );
        return;
    }
  
    if ( IS_NPC(victim) )
    {
        send_to_char( "Not on NPC's.\n", ch );
        return;
    }
    
    if ( get_trust( victim ) >= get_trust( ch ) )
    {
        send_to_char( "You failed.\n", ch );
        return;
    }
    
    if ( BV_IS_SET(victim->pcdata->flags, PCFLAG_NOTITLE) )
    {
        BV_REMOVE_BIT(victim->pcdata->flags, PCFLAG_NOTITLE);
        send_to_char( "You can set your own title again.\n", victim );
        send_to_char( "NOTITLE removed.\n", ch );
    }
    else
    {
        BV_SET_BIT(victim->pcdata->flags, PCFLAG_NOTITLE);
        SPRINTF( buf, "%s", victim->name );
        set_title( victim, buf );   
        send_to_char( "You can't set your own title!\n", victim );
        send_to_char( "NOTITLE set.\n", ch );
    }
    
    return;
}

void do_silence( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Silence whom?", ch );
	return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n", ch );
	return;
    }

    if ( IS_NPC(victim) )
    {
	send_to_char( "Not on NPC's.\n", ch );
	return;
    }

    if ( get_trust( victim ) >= get_trust( ch ) )
    {
	send_to_char( "You failed.\n", ch );
	return;
    }

    if ( BV_IS_SET(victim->act, PLR_SILENCE) )
    {
	send_to_char( "Player already silenced, use unsilence to remove.\n", ch );
    }
    else
    {
	BV_SET_BIT(victim->act, PLR_SILENCE);
	send_to_char( "You can't use channels!\n", victim );
	send_to_char( "SILENCE set.\n", ch );
    }

    return;
}

void do_unsilence( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Unsilence whom?\n", ch );
	return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n", ch );
	return;
    }

    if ( IS_NPC(victim) )
    {
	send_to_char( "Not on NPC's.\n", ch );
	return;
    }

    if ( get_trust( victim ) >= get_trust( ch ) )
    {
	send_to_char( "You failed.\n", ch );
	return;
    }

    if ( BV_IS_SET(victim->act, PLR_SILENCE) )
    {
	BV_REMOVE_BIT(victim->act, PLR_SILENCE);
	send_to_char( "You can use channels again.\n", victim );
	send_to_char( "SILENCE removed.\n", ch );
    }
    else
    {
	send_to_char( "That player is not silenced.\n", ch );
    }

    return;
}




void do_peace( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *rch;

    act( AT_IMMORT, "$n booms, 'PEACE!'", ch, NULL, NULL, TO_ROOM );
    for ( rch = ch->in_room->first_person; rch; rch = rch->next_in_room )
    {
	if ( rch->fighting )
	{
	    stop_fighting( rch, TRUE );
	    do_sit( rch, "" );
	}
       
        /* Added by Narn, Nov 28/95 */
        stop_hating( rch );
        stop_hunting( rch );
        stop_fearing( rch );
    }

    send_to_char( "Ok.\n", ch );
    return;
}



BAN_DATA *		first_ban;
BAN_DATA *		last_ban;

void save_banlist( void )
{
  BAN_DATA *pban;
  FILE *fp;

  FCLOSE( fpReserve );
  if ( !(fp = fopen( SYSTEM_DIR BAN_LIST, "w" )) )
  {
    bug( "%s: Cannot open " BAN_LIST, __func__ );
    perror(BAN_LIST);
    fpReserve = fopen( NULL_FILE, "r" );
    return;
  }
  for ( pban = first_ban; pban; pban = pban->next )
    fprintf( fp, "%d %s~~%s~\n", pban->level, pban->name, pban->ban_time );
  fprintf( fp, "-1\n" );
  FCLOSE( fp );
  fpReserve = fopen( NULL_FILE, "r" );
  return;
}
  
  

void do_ban( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    BAN_DATA *pban;
    int bnum;

    if ( IS_NPC(ch) )
	return;

    argument = one_argument( argument, arg );

    set_pager_color( AT_PLAIN, ch );
    if ( arg[0] == '\0' )
    {
	send_to_pager( "Banned sites:\n", ch );
	send_to_pager( "[ #] (Lv) Time                     Site\n", ch );
	send_to_pager( "---- ---- ------------------------ ---------------\n", ch );
	for ( pban = first_ban, bnum = 1; pban; pban = pban->next, bnum++ )
	    pager_printf(ch, "[%2d] (%2d) %-24s %s\n", bnum,
	            pban->level, pban->ban_time, pban->name);
	return;
    }
    
    /* People are gonna need .# instead of just # to ban by just last
       number in the site ip.                               -- Altrag */
    if ( is_number(arg) )
    {
      for ( pban = first_ban, bnum = 1; pban; pban = pban->next, bnum++ )
        if ( bnum == atoi(arg) )
          break;
      if ( !pban )
      {
        do_ban(ch, "");
        return;
      }
      argument = one_argument(argument, arg);
      if ( arg[0] == '\0' )
      {
        do_ban( ch, "help" );
        return;
      }
      if ( !str_cmp(arg, "level") )
      {
        argument = one_argument(argument, arg);
        if ( arg[0] == '\0' || !is_number(arg) )
        {
          do_ban( ch, "help" );
          return;
        }
        if ( atoi(arg) < 1 || atoi(arg) > LEVEL_SUPREME )
        {
          ch_printf(ch, "Level range: 1 - %d.\n", LEVEL_SUPREME);
          return;
        }
        pban->level = atoi(arg);
        send_to_char( "Ban level set.\n", ch );
      }
      else if ( !str_cmp(arg, "newban") )
      {
        pban->level = 1;
        send_to_char( "New characters banned.\n", ch );
      }
      else if ( !str_cmp(arg, "mortal") )
      {
        pban->level = LEVEL_AVATAR;
        send_to_char( "All mortals banned.\n", ch );
      }
      else if ( !str_cmp(arg, "total") )
      {
        pban->level = LEVEL_SUPREME;
        send_to_char( "Everyone banned.\n", ch );
      }
      else
      {
        do_ban(ch, "help");
        return;
      }
      save_banlist( );
      return;
    }
    
    if ( !str_cmp(arg, "help") )
    {
      send_to_char( "Syntax: ban <site address>\n", ch );
      send_to_char( "Syntax: ban <ban number> <level <lev>|newban|mortal|"
                    "total>\n", ch );
      return;
    }

    for ( pban = first_ban; pban; pban = pban->next )
    {
	if ( !str_cmp( arg, pban->name ) )
	{
	    send_to_char( "That site is already banned!\n", ch );
	    return;
	}
    }

    CREATE( pban, BAN_DATA, 1 );
    LINK( pban, first_ban, last_ban, next, prev );
    pban->name	= str_dup( arg );
    pban->level = LEVEL_AVATAR;
    SPRINTF(buf, "%24.24s", ctime(&current_time));
    pban->ban_time = str_dup( buf );
    save_banlist( );
    send_to_char( "Ban created.  Mortals banned from site.\n", ch );
    return;
}


void do_allow( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    BAN_DATA *pban;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Remove which site from the ban list?\n", ch );
	return;
    }

    for ( pban = first_ban; pban; pban = pban->next )
    {
	if ( !str_cmp( arg, pban->name ) )
	{
	    UNLINK( pban, first_ban, last_ban, next, prev );
	    if ( pban->ban_time )
	      STR_DISPOSE(pban->ban_time);
	    STR_DISPOSE( pban->name );
	    DISPOSE( pban );
	    save_banlist( );
	    send_to_char( "Site no longer banned.\n", ch );
	    return;
	}
    }

    send_to_char( "Site is not banned.\n", ch );
    return;
}



void do_wizlock( CHAR_DATA *ch, char *argument )
{
    extern bool wizlock;
    wizlock = !wizlock;

    if ( wizlock )
	send_to_char( "Game wizlocked.\n", ch );
    else
	send_to_char( "Game un-wizlocked.\n", ch );

    return;
}


void do_noresolve( CHAR_DATA *ch, char *argument )
{
    sysdata.NO_NAME_RESOLVING = !sysdata.NO_NAME_RESOLVING;

    if ( sysdata.NO_NAME_RESOLVING )
	send_to_char( "Name resolving disabled.\n", ch );
    else
	send_to_char( "Name resolving enabled.\n", ch );

    return;
}


void do_users( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    int count;
    char arg[MAX_INPUT_LENGTH];

    one_argument (argument, arg);
    count	= 0;
    buf[0]	= '\0';

    set_pager_color( AT_PLAIN, ch );
    SPRINTF(buf, 
"Desc|Con|Idle| Port | Player@HostIP                 ");
   STRAPP(buf, "\n");
   STRAPP(buf, "----+---+----+------+-------------------------------");
   STRAPP(buf, "\n");
   send_to_pager(buf, ch);

    for ( d = first_descriptor; d; d = d->next )
    {
      if (arg[0] == '\0')
      {     
	if (  get_trust(ch) >= LEVEL_SUPREME
	||   (d->character && can_see( ch, d->character )) )
	{
	    count++;
	    SPRINTF( buf,
	     " %3d| %2d|%4d|%6d| %s@%s ",
		d->descriptor,
		d->connected,
		d->idle / 4,
		d->port,
		d->original  ? d->original->name  :
		d->character ? d->character->name : "(none)",
		d->hostip );
	    if ( ch->top_level >= LEVEL_GOD && ( !d->character || d->character->top_level <= LEVEL_GOD ) )
	      STRAPP( buf, " (%s@%s)", d->user, d->host  );
	    STRAPP(buf, "\n");
	    send_to_pager( buf, ch );
	}
      }
      else
      {
	if ( (get_trust(ch) >= LEVEL_SUPREME
	||   (d->character && can_see( ch, d->character )) )
        &&   ( !str_prefix( arg, d->host ) 
	||   ( d->character && !str_prefix( arg, d->character->name ) ) ) )
	{
	    count++;
	    pager_printf( ch,
	     " %3d| %2d|%4d|%6d| %-12s@%-16s ",
		d->descriptor,
		d->connected,
		d->idle / 4,
		d->port,
		d->original  ? d->original->name  :
		d->character ? d->character->name : "(none)",
		d->host
		);
	    buf[0] = '\0';
	    if (get_trust(ch) >= LEVEL_GOD)
	      SPRINTF(buf, "| %s", d->user);
	    STRAPP(buf, "\n");
	    send_to_pager( buf, ch );
	}
      }
    }
    pager_printf( ch, "%d user%s.\n", count, count == 1 ? "" : "s" );
    return;
}



/*
 * Thanks to Grodyn for pointing out bugs in this function.
 */
void do_force( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    bool mobsonly; 
    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' || argument[0] == '\0' )
    {
	send_to_char( "Force whom to do what?\n", ch );
	return;
    }

    mobsonly = get_trust( ch ) < sysdata.level_forcepc; 

    if ( !str_cmp( arg, "all" ) )
    {
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;

        if ( mobsonly )
        {
	  send_to_char( "Force whom to do what?\n", ch );
	  return;
        } 

	for ( vch = first_char; vch; vch = vch_next )
	{
	    vch_next = vch->next;

	    if ( !IS_NPC(vch) && get_trust( vch ) < get_trust( ch ) )
	    {
		act( AT_IMMORT, "$n forces you to '$t'.", ch, argument, vch, TO_VICT );
		interpret( vch, argument );
	    }
	}
    }
    else
    {
	CHAR_DATA *victim;

	if ( ( victim = get_char_world( ch, arg ) ) == NULL )
	{
	    send_to_char( "They aren't here.\n", ch );
	    return;
	}

	if ( victim == ch )
	{
	    send_to_char( "Aye aye, right away!\n", ch );
	    return;
	}

	if ( ( get_trust( victim ) >= get_trust( ch ) ) 
          || ( mobsonly && !IS_NPC( victim ) ) )
	{
	    send_to_char( "Do it yourself!\n", ch );
	    return;
	}

    act( AT_IMMORT, "$n forces you to '$t'.", ch, argument, victim, TO_VICT );
	interpret( victim, argument );
    }

    send_to_char( "Ok.\n", ch );
    return;
}


void do_invis( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    sh_int level;
    
    /*
    if ( IS_NPC(ch))
	return;
    */

    argument = one_argument( argument, arg );
    if ( *arg && arg[0] != '\0' )
    {
	if ( !is_number( arg ) )
	{
	   send_to_char( "Usage: invis | invis <level>\n", ch );
	   return;
	}
	level = atoi( arg );
	if ( level < 2 || level > ch->top_level)
	{
	    send_to_char( "Invalid level.\n", ch );
	    return;
	}

    if (!IS_NPC(ch))
    {
        ch->pcdata->wizinvis = level;
        ch_printf( ch, "Wizinvis level set to %d.\n", level );
    }
    
    if (IS_NPC(ch))       
    {
        ch->mobinvis = level;
        ch_printf( ch, "Mobinvis level set to %d.\n", level );
    }
	return;
    }
    
    if( IS_NPC( ch ) )
    {
        if( ch->mobinvis < 2 )
            ch->mobinvis = ch->top_level;
        return;
    }

    if( ch->pcdata->wizinvis < 2 )
        ch->pcdata->wizinvis = ch->top_level;

    if ( BV_IS_SET(ch->act, PLR_WIZINVIS) )
    {
	BV_REMOVE_BIT(ch->act, PLR_WIZINVIS);
	act( AT_IMMORT, "$n slowly fades into existence.", ch, NULL, NULL, TO_ROOM );
	send_to_char( "You slowly fade back into existence.\n", ch );
    }
    else
    {
	BV_SET_BIT(ch->act, PLR_WIZINVIS);
	act( AT_IMMORT, "$n slowly fades into thin air.", ch, NULL, NULL, TO_ROOM );
	send_to_char( "You slowly vanish into thin air.\n", ch );
    }

    return;
}


void do_holylight( CHAR_DATA *ch, char *argument )
{
    if ( IS_NPC(ch) )
	return;

    if ( BV_IS_SET(ch->act, PLR_HOLYLIGHT) )
    {
	BV_REMOVE_BIT(ch->act, PLR_HOLYLIGHT);
	send_to_char( "Holy light mode off.\n", ch );
    }
    else
    {
	BV_SET_BIT(ch->act, PLR_HOLYLIGHT);
	send_to_char( "Holy light mode on.\n", ch );
    }

    return;
}

void do_rassign( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    sh_int  r_lo, r_hi;
    CHAR_DATA *victim;
    
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );
    r_lo = atoi( arg2 );  r_hi = atoi( arg3 );

    if ( arg1[0] == '\0' || r_lo < 0 || r_hi < 0 )
    {
	send_to_char( "Syntax: assign <who> <low> <high>\n", ch );
	return;
    }
    if ( (victim = get_char_world( ch, arg1 )) == NULL )
    {
	send_to_char( "They don't seem to be around.\n", ch );
	return;
    }
    if ( IS_NPC( victim ) || get_trust( victim ) < LEVEL_AVATAR )
    {
	send_to_char( "They wouldn't know what to do with a room range.\n", ch );
	return;
    }
    if ( r_lo > r_hi )
    {
	send_to_char( "Unacceptable room range.\n", ch );
	return;
    }
    if ( r_lo == 0 )
       r_hi = 0;
    victim->pcdata->r_range_lo = r_lo;
    victim->pcdata->r_range_hi = r_hi;                      
    assign_area( victim );
    send_to_char( "Done.\n", ch );
    ch_printf( victim, "%s has assigned you the room range %d - %d.\n",
		ch->name, r_lo, r_hi );
    assign_area( victim );	/* Put back by Thoric on 02/07/96 */
    if ( !victim->pcdata->area )
    {
	bug( "rassign: assign_area failed", 0 );
    	return;
    }

    if (r_lo == 0)				/* Scryn 8/12/95 */
    {
	REMOVE_BIT ( victim->pcdata->area->status, AREA_LOADED );
	SET_BIT( victim->pcdata->area->status, AREA_DELETED );
    }
    else
    {
        SET_BIT( victim->pcdata->area->status, AREA_LOADED );
	REMOVE_BIT( victim->pcdata->area->status, AREA_DELETED );
    }
    return;
}

bool check_area_conflict( AREA_DATA * area, int low_range, int hi_range )
{
   if( low_range < area->low_r_vnum && area->low_r_vnum < hi_range )
      return TRUE;
   if( low_range < area->low_m_vnum && area->low_m_vnum < hi_range )
      return TRUE;
   if( low_range < area->low_o_vnum && area->low_o_vnum < hi_range )
      return TRUE;

   if( low_range < area->hi_r_vnum && area->hi_r_vnum < hi_range )
      return TRUE;
   if( low_range < area->hi_m_vnum && area->hi_m_vnum < hi_range )
      return TRUE;
   if( low_range < area->hi_o_vnum && area->hi_o_vnum < hi_range )
      return TRUE;

   if( ( low_range >= area->low_r_vnum ) && ( low_range <= area->hi_r_vnum ) )
      return TRUE;
   if( ( low_range >= area->low_m_vnum ) && ( low_range <= area->hi_m_vnum ) )
      return TRUE;
   if( ( low_range >= area->low_o_vnum ) && ( low_range <= area->hi_o_vnum ) )
      return TRUE;

   if( ( hi_range <= area->hi_r_vnum ) && ( hi_range >= area->low_r_vnum ) )
      return TRUE;
   if( ( hi_range <= area->hi_m_vnum ) && ( hi_range >= area->low_m_vnum ) )
      return TRUE;
   if( ( hi_range <= area->hi_o_vnum ) && ( hi_range >= area->low_o_vnum ) )
      return TRUE;

   return FALSE;
}

/* Runs the entire list, easier to call in places that have to check them all */
bool check_area_conflicts( int lo, int hi )
{
   AREA_DATA *area;

   for( area = first_area; area; area = area->next )
      if( check_area_conflict( area, lo, hi ) )
         return TRUE;

   for( area = first_build; area; area = area->next )
      if( check_area_conflict( area, lo, hi ) )
         return TRUE;

   return FALSE;
}

void do_vassign( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    sh_int  r_lo, r_hi;
    CHAR_DATA *victim;
    
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );
    r_lo = atoi( arg2 );  r_hi = atoi( arg3 );

    if ( arg1[0] == '\0' || r_lo < 0 || r_hi < 0 )
    {
        send_to_char( "Syntax: vassign <who> <low> <high>\n", ch );
        return;
    }
    if ( (victim = get_char_world( ch, arg1 )) == NULL )
    {
        send_to_char( "They don't seem to be around.\n", ch );
        return;
    }
    if ( IS_NPC( victim ) || get_trust( victim ) < LEVEL_CREATOR )
    {
        send_to_char( "They wouldn't know what to do with a vnum range.\n", ch );
        return;
    }
    if ( r_lo > r_hi )
    {
        send_to_char( "Unacceptable room range.\n", ch );
        return;
    }
    if( check_area_conflicts( r_lo, r_hi ) )
    {
        send_to_char( "That vnum range conflicts with another area. Check the zones or vnums command.\r\n", ch );
        return;
    }    
    if ( r_lo == 0 )
       r_hi = 0;
    victim->pcdata->r_range_lo = r_lo;
    victim->pcdata->r_range_hi = r_hi;
    victim->pcdata->o_range_lo = r_lo;
    victim->pcdata->o_range_hi = r_hi;
    victim->pcdata->m_range_lo = r_lo;
    victim->pcdata->m_range_hi = r_hi;
                     
    assign_area( victim );
    send_to_char( "Done.\n", ch );
    ch_printf( victim, "%s has assigned you the vnum range %d - %d.\n",
		ch->name, r_lo, r_hi );
    assign_area( victim );	/* Put back by Thoric on 02/07/96 */
    if ( !victim->pcdata->area )
    {
	    bug( "%s: assign_area failed", __func__ );
    	return;
    }

    if (r_lo == 0)				/* Scryn 8/12/95 */
    {
        REMOVE_BIT ( victim->pcdata->area->status, AREA_LOADED );
        SET_BIT( victim->pcdata->area->status, AREA_DELETED );
    }
    else
    {
        SET_BIT( victim->pcdata->area->status, AREA_LOADED );
        REMOVE_BIT( victim->pcdata->area->status, AREA_DELETED );
    }
    return;
}

void do_oassign( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    sh_int  o_lo, o_hi;
    CHAR_DATA *victim;
    
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );
    o_lo = atoi( arg2 );  o_hi = atoi( arg3 );

    if ( arg1[0] == '\0' || o_lo < 0 || o_hi < 0 )
    {
	send_to_char( "Syntax: oassign <who> <low> <high>\n", ch );
	return;
    }
    if ( (victim = get_char_world( ch, arg1 )) == NULL )
    {
	send_to_char( "They don't seem to be around.\n", ch );
	return;
    }
    if ( IS_NPC( victim ) || get_trust( victim ) < LEVEL_SAVIOR )
    {
	send_to_char( "They wouldn't know what to do with an object range.\n", ch );
	return;
    }
    if ( o_lo > o_hi )
    {
	send_to_char( "Unacceptable object range.\n", ch );
	return;
    }
    victim->pcdata->o_range_lo = o_lo;
    victim->pcdata->o_range_hi = o_hi;
    assign_area( victim );
    send_to_char( "Done.\n", ch );
    ch_printf( victim, "%s has assigned you the object vnum range %d - %d.\n",
		ch->name, o_lo, o_hi );
    return;
}

void do_massign( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    sh_int  m_lo, m_hi;
    CHAR_DATA *victim;
    
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );
    m_lo = atoi( arg2 );  m_hi = atoi( arg3 );

    if ( arg1[0] == '\0' || m_lo < 0 || m_hi < 0 )
    {
	send_to_char( "Syntax: massign <who> <low> <high>\n", ch );
	return;
    }
    if ( (victim = get_char_world( ch, arg1 )) == NULL )
    {
	send_to_char( "They don't seem to be around.\n", ch );
	return;
    }
    if ( IS_NPC( victim ) || get_trust( victim ) < LEVEL_SAVIOR )
    {
	send_to_char( "They wouldn't know what to do with a monster range.\n", ch );
	return;
    }
    if ( m_lo > m_hi )
    {
	send_to_char( "Unacceptable monster range.\n", ch );
	return;
    }
    victim->pcdata->m_range_lo = m_lo;
    victim->pcdata->m_range_hi = m_hi;
    assign_area( victim );
    send_to_char( "Done.\n", ch );
    ch_printf( victim, "%s has assigned you the monster vnum range %d - %d.\n",
		ch->name, m_lo, m_hi );
    return;
}

void do_cmdtable( CHAR_DATA *ch, char *argument )
{
    int hash, cnt;
    CMDTYPE *cmd;
 
    set_pager_color( AT_PLAIN, ch );
    send_to_pager("Commands and Number of Uses This Run\n", ch);
 
    for ( cnt = hash = 0; hash < 126; hash++ )
	for ( cmd = command_hash[hash]; cmd; cmd = cmd->next )
	{
	    if ((++cnt)%4)
		pager_printf(ch,"%-6.6s %4d\t",cmd->name,cmd->userec.num_uses);
	    else
		pager_printf(ch,"%-6.6s %4d\n", cmd->name,cmd->userec.num_uses );
	}
    return;
}

/*
 * Load up a player file
 */
void do_loadup( CHAR_DATA *ch, char *argument )
{
    char fname[1024];
    char name[256];
    struct stat fst;
//    bool loaded;
    DESCRIPTOR_DATA *d;
    int old_room_vnum;
    char buf[MAX_STRING_LENGTH];

    one_argument( argument, name );
    if ( name[0] == '\0' )
    {
	send_to_char( "Usage: loadup <playername>\n", ch );
	return;
    }

    name[0] = UPPER(name[0]);

    SPRINTF( fname, "%s%c/%s", PLAYER_DIR, tolower(name[0]),
			capitalize( name ) );
    if ( stat( fname, &fst ) != -1 )
    {
	CREATE( d, DESCRIPTOR_DATA, 1 );
	d->next = NULL;
	d->prev = NULL;
	d->connected = CON_GET_NAME;
	d->outsize = 2000;
	CREATE( d->outbuf, char, d->outsize );
	
	load_char_obj( d, name, FALSE );
	add_char( d->character );
        old_room_vnum = d->character->in_room->vnum;
	char_to_room( d->character, ch->in_room );

    if ( d->character->plr_home != NULL )
    	{
	 char filename[256];
         FILE *fph;
         ROOM_INDEX_DATA *storeroom = d->character->plr_home;
         OBJ_DATA *obj;
         OBJ_DATA *obj_next;
           
         for ( obj = storeroom->first_content; obj; obj = obj_next )
	 {
	    obj_next = obj->next_content;
	    extract_obj( obj );
	 }

	 SPRINTF( filename, "%s%c/%s.home", PLAYER_DIR, tolower(d->character->name[0]),
				 capitalize( d->character->name ) );
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
		    bug( d->character->name, 0 );
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
		    bug( d->character->name, 0 );
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


	if ( get_trust(d->character) >= get_trust( ch ) )
	{
	   do_say( d->character, "Do *NOT* disturb me again!" );
	   send_to_char( "I think you'd better leave that player alone!\n", ch );
	   d->character->desc	= NULL;
	   do_quit( d->character, "" );
	   return;	   
	}
	d->character->desc	= NULL;
	d->character->retran    = old_room_vnum;
	d->character		= NULL;	
	STR_DISPOSE( d->outbuf );
	DISPOSE( d );
	ch_printf(ch, "Player %s loaded from room %d.\n", capitalize( name ),old_room_vnum );
	SPRINTF(buf, "%s appears from nowhere, eyes glazed over.\n", capitalize( name ) );
        act( AT_IMMORT, buf, ch, NULL, NULL, TO_ROOM );

	send_to_char( "Done.\n", ch );
	return;
    }
    /* else no player file */
    send_to_char( "No such player.\n", ch );
    return;
}

void do_fixchar( CHAR_DATA *ch, char *argument )
{
    char name[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, name );
    if ( name[0] == '\0' )
    {
	send_to_char( "Usage: fixchar <playername>\n", ch );
	return;
    }
    victim = get_char_room( ch, name );
    if ( !victim )
    {
	send_to_char( "They're not here.\n", ch );
	return;
    }
    fix_char( victim );
/*  victim->armor	= 100;
    victim->mod_str	= 0;
    victim->mod_dex	= 0;
    victim->mod_wis	= 0;
    victim->mod_int	= 0;
    victim->mod_con	= 0;
    victim->mod_cha	= 0;
    victim->mod_lck	= 0;
    victim->damroll	= 0;
    victim->hitroll	= 0;
    victim->alignment	= URANGE( -1000, victim->alignment, 1000 );
    victim->saving_spell_staff = 0; */
    send_to_char( "Done.\n", ch );
}

void do_newbieset( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    CHAR_DATA *victim;
        
    argument = one_argument( argument, arg1 );
    argument = one_argument (argument, arg2);

    if ( arg1[0] == '\0' )
    {
	send_to_char( "Syntax: newbieset <char>.\n", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg1 ) ) == NULL )
    {
	send_to_char( "That player is not here.\n", ch);
	return;
    }

    if ( IS_NPC(victim) )
    {
	send_to_char( "Not on NPC's.\n", ch );
	return;
    }

    if ( ( victim->top_level < 1 ) || ( victim->top_level > 5 ) )
    {
     send_to_char( "Level of victim must be 1 to 5.\n", ch );
	return;
    }
     obj = create_object( get_obj_index(OBJ_VNUM_SCHOOL_SHIELD), 1 );
     obj_to_char(obj, victim);

     obj = create_object( get_obj_index(OBJ_VNUM_SCHOOL_DAGGER), 1 );
     obj_to_char(obj, victim);
  
     /* Added by Brittany, on Nov. 24, 1996. The object is the adventurer's 
          guide to the realms of despair, part of academy.are. */
     {
       OBJ_INDEX_DATA *obj_ind = get_obj_index( 10333 );
       if ( obj_ind != NULL )
       {
         obj = create_object( obj_ind, 1 );
         obj_to_char( obj, victim );
       }
     }

/* Added the burlap sack to the newbieset.  The sack is part of sgate.are
   called Spectral Gate.  Brittany */

     {

       OBJ_INDEX_DATA *obj_ind = get_obj_index( 123 );
       if ( obj_ind != NULL )
       {
         obj = create_object( obj_ind, 1 );
         obj_to_char( obj, victim );
       }
     }

    act( AT_IMMORT, "$n has equipped you with a newbieset.", ch, NULL, victim, TO_VICT);
    ch_printf( ch, "You have re-equipped %s.\n", victim->name );
    return;
}

/*
 * Extract area names from "input" string and place result in "output" string
 * e.g. "aset joe.are sedit susan.are cset" --> "joe.are susan.are"
 * - Gorog
 */
void extract_area_names (char *inp, char *out, long unsigned out_size)
{
char buf[MAX_INPUT_LENGTH], *pbuf=buf;
int  len;

*out='\0';
while (inp && *inp)
   {
   inp = one_argument(inp, buf);
   if ( (len=strlen(buf)) >= 5 && !strcmp(".are", pbuf+len-4) )
       {
            STRLCAT( out, " ", out_size );
         STRLCAT( out, buf, out_size );
       }
   }
}

/*
 * Remove area names from "input" string and place result in "output" string
 * e.g. "aset joe.are sedit susan.are cset" --> "aset sedit cset"
 * - Gorog
 */
void remove_area_names (char *inp, char *out, long unsigned out_size)
{
    char buf[MAX_INPUT_LENGTH], *pbuf=buf;
    int  len;

    *out='\0';
    while( inp && *inp )
    {
        inp = one_argument( inp, buf );
        if( ( len = strlen( buf ) ) < 5 || strcmp( ".are", pbuf + len - 4 ) )
        {
            if( *out )
                STRLCAT( out, " ", out_size );
            STRLCAT( out, buf, out_size );
        }
    }
}

void do_bestowarea( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    int  arg_len;

    argument = one_argument( argument, arg );

    if ( get_trust (ch) < LEVEL_SUB_IMPLEM )
       {
       send_to_char( "Sorry...\n", ch );
       return;
       }

    if ( !*arg )
        {
        send_to_char(
        "Syntax:\n"
        "bestowarea <victim> <filename>.are\n"
        "bestowarea <victim> none             removes bestowed areas\n"
        "bestowarea <victim> list             lists bestowed areas\n"
        "bestowarea <victim>                  lists bestowed areas\n", ch);
        return;
        }

    if ( !(victim = get_char_world( ch, arg )) )
        {
        send_to_char( "They aren't here.\n", ch );
        return;
        }

    if ( IS_NPC( victim ) )
        {
        send_to_char( "You can't give special abilities to a mob!\n", ch );
        return;
        }

    if ( get_trust(victim) < LEVEL_IMMORTAL )
        {
        send_to_char( "They aren't an immortal.\n", ch );
        return;
        }

    if (!victim->pcdata->bestowments)
       victim->pcdata->bestowments = str_dup("");

    if ( !*argument || !str_cmp (argument, "list") )
       {
       extract_area_names (victim->pcdata->bestowments, buf, sizeof(buf));
       ch_printf( ch, "Bestowed areas: %s\n", buf);
       return;
       }

    if ( !str_cmp (argument, "none") )
       {
       remove_area_names (victim->pcdata->bestowments, buf, sizeof(buf));
       STR_DISPOSE( victim->pcdata->bestowments );
       victim->pcdata->bestowments = str_dup( buf );
       send_to_char( "Done.\n", ch);
       return;
       }

    arg_len = strlen(argument);
    if ( arg_len < 5 
    || argument[arg_len-4] != '.' || argument[arg_len-3] != 'a'
    || argument[arg_len-2] != 'r' || argument[arg_len-1] != 'e' )
    {
        send_to_char( "You can only bestow an area name\n", ch );
        send_to_char( "E.G. bestow joe sam.are\n", ch );
        return;
    }

    SPRINTF( buf, "%s %s", victim->pcdata->bestowments, argument );
    STR_DISPOSE( victim->pcdata->bestowments );
    victim->pcdata->bestowments = str_dup( buf );
    ch_printf( victim, "%s has bestowed on you the area: %s\n", 
             ch->name, argument );
    send_to_char( "Done.\n", ch );
}

void do_bestow( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Bestow whom with what?\n", ch );
	return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n", ch );
	return;
    }

    if ( IS_NPC( victim ) )
    {
	send_to_char( "You can't give special abilities to a mob!\n", ch );
	return;
    }

    if ( get_trust( victim ) > get_trust( ch ) )
    {
	send_to_char( "You aren't powerful enough...\n", ch );
	return;
    }

    if (!victim->pcdata->bestowments)
      victim->pcdata->bestowments = str_dup("");

    if ( argument[0] == '\0' || !str_cmp( argument, "list" ) )
    {
        ch_printf( ch, "Current bestowed commands on %s: %s.\n",
                      victim->name, victim->pcdata->bestowments );
        return;
    }

    if ( !str_cmp( argument, "none" ) )
    {
        STR_DISPOSE( victim->pcdata->bestowments );
	victim->pcdata->bestowments = str_dup("");
        ch_printf( ch, "Bestowments removed from %s.\n", victim->name );
        ch_printf( victim, "%s has removed your bestowed commands.\n", ch->name );
        check_switch( victim, FALSE );        
        return;
    }

    SPRINTF( buf, "%s %s", victim->pcdata->bestowments, argument );
    STR_DISPOSE( victim->pcdata->bestowments );
    victim->pcdata->bestowments = str_dup( buf );
    ch_printf( victim, "%s has bestowed on you the command(s): %s\n", 
             ch->name, argument );
    send_to_char( "Done.\n", ch );
}

struct tm *update_time ( struct tm *old_time )
{
   time_t time;

   time = mktime(old_time); 
   return localtime(&time);
}

void do_set_boot_time( CHAR_DATA *ch, char *argument)
{
   char arg[MAX_INPUT_LENGTH];
   char arg1[MAX_INPUT_LENGTH];
   bool check;
 
   check = FALSE;

   argument = one_argument(argument, arg);

    if ( arg[0] == '\0' )
    {
	send_to_char( "Syntax: setboot time {hour minute <day> <month> <year>}\n", ch);
	send_to_char( "        setboot manual {0/1}\n", ch);
	send_to_char( "        setboot default\n", ch); 
        ch_printf( ch, "Boot time is currently set to %s, manual bit is set to %d\n"
	,reboot_time, set_boot_time->manual );
	return;
    }

    if ( !str_cmp(arg, "time") )
    {
      struct tm *now_time;
      
      argument = one_argument(argument, arg);
      argument = one_argument(argument, arg1);
      if ( !*arg || !*arg1 || !is_number(arg) || !is_number(arg1) )
      {
	send_to_char("You must input a value for hour and minute.\n", ch);
 	return;
      }
      now_time = localtime(&current_time);

      if ( (now_time->tm_hour = atoi(arg)) < 0 || now_time->tm_hour > 23 )
      {
        send_to_char("Valid range for hour is 0 to 23.\n", ch);
        return;
      }
 
      if ( (now_time->tm_min = atoi(arg1)) < 0 || now_time->tm_min > 59 )
      {
        send_to_char("Valid range for minute is 0 to 59.\n", ch);
        return;
      }

      argument = one_argument(argument, arg);
      if ( *arg != '\0' && is_number(arg) )
      {
        if ( (now_time->tm_mday = atoi(arg)) < 1 || now_time->tm_mday > 31 )
        {
	  send_to_char("Valid range for day is 1 to 31.\n", ch);
	  return;
        }
        argument = one_argument(argument, arg);
        if ( *arg != '\0' && is_number(arg) )
        {
          if ( (now_time->tm_mon = atoi(arg)) < 1 || now_time->tm_mon > 12 )
          {
            send_to_char( "Valid range for month is 1 to 12.\n", ch );
            return;
          }
          now_time->tm_mon--;
          argument = one_argument(argument, arg);
          if ( (now_time->tm_year = atoi(arg)-1900) < 0 ||
                now_time->tm_year > 199 )
          {
            send_to_char( "Valid range for year is 1900 to 2099.\n", ch );
            return;
          }
        }
      }
      now_time->tm_sec = 0;
      if ( mktime(now_time) < current_time )
      {
        send_to_char( "You can't set a time previous to today!\n", ch );
        return;
      }
      if (set_boot_time->manual == 0)
 	set_boot_time->manual = 1;
      new_boot_time = update_time(now_time);
      new_boot_struct = *new_boot_time;
      new_boot_time = &new_boot_struct;
      reboot_check(mktime(new_boot_time));
      get_reboot_string();

      ch_printf(ch, "Boot time set to %s\n", reboot_time);
      check = TRUE;
    }  
    else if ( !str_cmp(arg, "manual") )
    {
      argument = one_argument(argument, arg1);
      if (arg1[0] == '\0')
      {
	send_to_char("Please enter a value for manual boot on/off\n", ch);
	return;
      }
      
      if ( !is_number(arg1))
      {
	send_to_char("Value for manual must be 0 (off) or 1 (on)\n", ch);
	return;
      }

      if (atoi(arg1) < 0 || atoi(arg1) > 1)
      {
	send_to_char("Value for manual must be 0 (off) or 1 (on)\n", ch);
	return;
      }
   
      set_boot_time->manual = atoi(arg1);
      ch_printf(ch, "Manual bit set to %s\n", arg1);
      check = TRUE;
      get_reboot_string();
      return;
    }

    else if (!str_cmp( arg, "default" ))
    {
      set_boot_time->manual = 0;
    /* Reinitialize new_boot_time */
      new_boot_time = localtime(&current_time);
      new_boot_time->tm_mday += 1;
      if (new_boot_time->tm_hour > 12)
      new_boot_time->tm_mday += 1; 
      new_boot_time->tm_hour = 6;
      new_boot_time->tm_min = 0;
      new_boot_time->tm_sec = 0;
      new_boot_time = update_time(new_boot_time);

      sysdata.DENY_NEW_PLAYERS = FALSE;

      send_to_char("Reboot time set back to normal.\n", ch);
      check = TRUE;
    }

    if (!check)
    {
      send_to_char("Invalid argument for setboot.\n", ch);
      return;
    }

    else
    {
      get_reboot_string();
      new_boot_time_t = mktime(new_boot_time);
    }
}
/* Online high level immortal command for displaying what the encryption
 * of a name/password would be, taking in 2 arguments - the name and the
 * password - can still only change the password if you have access to 
 * pfiles and the correct password

 Replace crypt() with SHA-256 hashing - AI/DV 3-12-26
 */ 
void do_form_password(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_STRING_LENGTH];
    char hashbuf[65]; // SHA-256 hex string buffer

    /* Get first argument */
    argument = one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char("Syntax: form_password <text>\n", ch);
        return;
    }

    /* Hash the argument with SHA-256 */
    sha256_hash(arg, hashbuf);

    /* Display the hash */
    ch_printf(ch, "SHA-256 hash of \"%s\" is: %s\n", arg, hashbuf);
}

/*
 * Purge a player file.  No more player.  -- Altrag
 */
void do_destro( CHAR_DATA *ch, char *argument )
{
  set_char_color( AT_RED, ch );
  send_to_char("If you want to destroy a character, spell it out!\n",ch);
  return;
}

/*
 * This could have other applications too.. move if needed. -- Altrag
 */
void close_area( AREA_DATA *pArea )
{
  extern ROOM_INDEX_DATA *room_index_hash[MAX_KEY_HASH];
  extern OBJ_INDEX_DATA   *obj_index_hash[MAX_KEY_HASH];
  extern MOB_INDEX_DATA   *mob_index_hash[MAX_KEY_HASH];
  CHAR_DATA *ech;
  CHAR_DATA *ech_next;
  OBJ_DATA *eobj;
  OBJ_DATA *eobj_next;
  int icnt;
  ROOM_INDEX_DATA *rid;
  ROOM_INDEX_DATA *rid_next;
  OBJ_INDEX_DATA *oid;
  OBJ_INDEX_DATA *oid_next;
  MOB_INDEX_DATA *mid;
  MOB_INDEX_DATA *mid_next;
  RESET_DATA *ereset;
  RESET_DATA *ereset_next;
  EXTRA_DESCR_DATA *eed;
  EXTRA_DESCR_DATA *eed_next;
  EXIT_DATA *exit;
  EXIT_DATA *exit_next;
  MPROG_ACT_LIST *mpact;
  MPROG_ACT_LIST *mpact_next;
  MPROG_DATA *mprog;
  MPROG_DATA *mprog_next;
  AFFECT_DATA *paf;
  AFFECT_DATA *paf_next;
  
  for ( ech = first_char; ech; ech = ech_next )
  {
    ech_next = ech->next;
    
    if ( ech->fighting )
      stop_fighting( ech, TRUE );
    if ( IS_NPC(ech) )
    {
      /* if mob is in area, or part of area. */
      if ( URANGE(pArea->low_m_vnum, ech->pIndexData->vnum,
                  pArea->hi_m_vnum) == ech->pIndexData->vnum ||
          (ech->in_room && ech->in_room->area == pArea) )
        extract_char( ech, TRUE );
      continue;
    }
    if ( ech->in_room && ech->in_room->area == pArea )
      do_recall( ech, "" );
  }
  for ( eobj = first_object; eobj; eobj = eobj_next )
  {
    eobj_next = eobj->next;
    /* if obj is in area, or part of area. */
    if ( URANGE(pArea->low_o_vnum, eobj->pIndexData->vnum,
                pArea->hi_o_vnum) == eobj->pIndexData->vnum ||
        (eobj->in_room && eobj->in_room->area == pArea) )
      extract_obj( eobj );
  }
  for ( icnt = 0; icnt < MAX_KEY_HASH; icnt++ )
  {
    for ( rid = room_index_hash[icnt]; rid; rid = rid_next )
    {
      rid_next = rid->next;
      
      for ( exit = rid->first_exit; exit; exit = exit_next )
      {
        exit_next = exit->next;
        if ( rid->area == pArea || exit->to_room->area == pArea )
        {
          STRFREE( exit->keyword );
          STRFREE( exit->description );
          UNLINK( exit, rid->first_exit, rid->last_exit, next, prev );
          DISPOSE( exit );
        }
      }
      if ( rid->area != pArea )
        continue;
      STRFREE(rid->name);
      STRFREE(rid->description);
      if ( rid->first_person )
      {
        bug( "close_area: room with people #%d", rid->vnum );
        for ( ech = rid->first_person; ech; ech = ech_next )
        {
          ech_next = ech->next_in_room;
          if ( ech->fighting )
            stop_fighting( ech, TRUE );
          if ( IS_NPC(ech) )
            extract_char( ech, TRUE );
          else
            do_recall( ech, "" );
        }
      }
      if ( rid->first_content )
      {
        bug( "close_area: room with contents #%d", rid->vnum );
        for ( eobj = rid->first_content; eobj; eobj = eobj_next )
        {
          eobj_next = eobj->next_content;
          extract_obj( eobj );
        }
      }
      for ( eed = rid->first_extradesc; eed; eed = eed_next )
      {
        eed_next = eed->next;
        STRFREE( eed->keyword );
        STRFREE( eed->description );
        DISPOSE( eed );
      }
      for ( mpact = rid->mpact; mpact; mpact = mpact_next )
      {
        mpact_next = mpact->next;
        STRFREE( mpact->buf );
        DISPOSE( mpact );
      }
      for ( mprog = rid->mudprogs; mprog; mprog = mprog_next )
      {
        mprog_next = mprog->next;
        STRFREE( mprog->arglist );
        STRFREE( mprog->comlist );
        DISPOSE( mprog );
      }
      if ( rid == room_index_hash[icnt] )
        room_index_hash[icnt] = rid->next;
      else
      {
        ROOM_INDEX_DATA *trid;
        
        for ( trid = room_index_hash[icnt]; trid; trid = trid->next )
          if ( trid->next == rid )
            break;
        if ( !trid )
          bug( "Close_area: rid not in hash list %d", rid->vnum );
        else
          trid->next = rid->next;
      }
      DISPOSE(rid);
    }
    
    for ( mid = mob_index_hash[icnt]; mid; mid = mid_next )
    {
      mid_next = mid->next;
      
      if ( mid->vnum < pArea->low_m_vnum || mid->vnum > pArea->hi_m_vnum )
        continue;
      
      STRFREE( mid->player_name );
      STRFREE( mid->short_descr );
      STRFREE( mid->long_descr  );
      STRFREE( mid->description );
      if ( mid->pShop )
      {
        UNLINK( mid->pShop, first_shop, last_shop, next, prev );
        DISPOSE( mid->pShop );
      }
      if ( mid->rShop )
      {
        UNLINK( mid->rShop, first_repair, last_repair, next, prev );
        DISPOSE( mid->rShop );
      }
      for ( mprog = mid->mudprogs; mprog; mprog = mprog_next )
      {
        mprog_next = mprog->next;
        STRFREE(mprog->arglist);
        STRFREE(mprog->comlist);
        DISPOSE(mprog);
      }
      if ( mid == mob_index_hash[icnt] )
        mob_index_hash[icnt] = mid->next;
      else
      {
        MOB_INDEX_DATA *tmid;
        
        for ( tmid = mob_index_hash[icnt]; tmid; tmid = tmid->next )
          if ( tmid->next == mid )
            break;
        if ( !tmid )
          bug( "Close_area: mid not in hash list %s", mid->vnum );
        else
          tmid->next = mid->next;
      }
      DISPOSE(mid);
    }
    
    for ( oid = obj_index_hash[icnt]; oid; oid = oid_next )
    {
      oid_next = oid->next;
      
      if ( oid->vnum < pArea->low_o_vnum || oid->vnum > pArea->hi_o_vnum )
        continue;
      
      STRFREE(oid->name);
      STRFREE(oid->short_descr);
      STRFREE(oid->description);
      STRFREE(oid->action_desc);

      for ( eed = oid->first_extradesc; eed; eed = eed_next )
      {
        eed_next = eed->next;
        STRFREE(eed->keyword);
        STRFREE(eed->description);
        DISPOSE(eed);
      }
      for ( paf = oid->first_affect; paf; paf = paf_next )
      {
        paf_next = paf->next;
        DISPOSE(paf);
      }
      for ( mprog = oid->mudprogs; mprog; mprog = mprog_next )
      {
        mprog_next = mprog->next;
        STRFREE(mprog->arglist);
        STRFREE(mprog->comlist);
        DISPOSE(mprog);
      }
      if ( oid == obj_index_hash[icnt] )
        obj_index_hash[icnt] = oid->next;
      else
      {
        OBJ_INDEX_DATA *toid;
        
        for ( toid = obj_index_hash[icnt]; toid; toid = toid->next )
          if ( toid->next == oid )
            break;
        if ( !toid )
          bug( "Close_area: oid not in hash list %s", oid->vnum );
        else
          toid->next = oid->next;
      }
      DISPOSE(oid);
    }
  }
  for ( ereset = pArea->first_reset; ereset; ereset = ereset_next )
  {
    ereset_next = ereset->next;
    DISPOSE(ereset);
  }
  STR_DISPOSE(pArea->name);
  STR_DISPOSE(pArea->filename);
  STRFREE(pArea->author);
  UNLINK( pArea, first_build, last_build, next, prev );
  UNLINK( pArea, first_asort, last_asort, next_sort, prev_sort );
  DISPOSE( pArea );
}

void do_destroy( CHAR_DATA *ch, char *argument )
{
  CHAR_DATA *victim;
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  char arg[MAX_INPUT_LENGTH];

  one_argument( argument, arg );
  if ( arg[0] == '\0' )
  {
      send_to_char( "Destroy what player file?\n", ch );
      return;
  }

  for ( victim = first_char; victim; victim = victim->next )
    if ( !IS_NPC(victim) && !str_cmp(victim->name, arg) )
      break;
  if ( !victim )
  {
    DESCRIPTOR_DATA *d;
    
    /* Make sure they aren't halfway logged in. */
    for ( d = first_descriptor; d; d = d->next )
      if ( (victim = d->character) && !IS_NPC(victim) &&
          !str_cmp(victim->name, arg) )
        break;
    if ( d )
      close_socket( d, TRUE );
  }
  else        
  {
    int x, y;
    
    quitting_char = victim;
    save_char_obj( victim );
    saving_char = NULL;
    extract_char( victim, TRUE );
    for ( x = 0; x < MAX_WEAR; x++ )
	for ( y = 0; y < MAX_LAYERS; y++ )
	    save_equipment[x][y] = NULL;
  }
  
  SPRINTF( buf, "%s%c/%s", PLAYER_DIR, tolower(arg[0]),
          capitalize( arg ) );
  SPRINTF( buf2, "%s%c/%s", BACKUP_DIR, tolower(arg[0]),
          capitalize( arg ) );
  if ( !rename( buf, buf2 ) )
  {
    AREA_DATA *pArea;
    
    set_char_color( AT_RED, ch );
    send_to_char( "Player destroyed.  Pfile saved in backup directory.\n", ch );
    SPRINTF( buf, "%s%s", GOD_DIR, capitalize(arg) );
    if ( !remove( buf ) )
      send_to_char( "Player's immortal data destroyed.\n", ch );
    else if ( errno != ENOENT )
    {
      ch_printf( ch, "Unknown error #%d - %s (immortal data).  Report to Thoric.\n",
              errno, strerror( errno ) );
      SPRINTF( buf2, "%s destroying %s", ch->name, buf );
      perror( buf2 );
    }

    SPRINTF( buf2, "%s.are", capitalize(arg) );
    for ( pArea = first_build; pArea; pArea = pArea->next )
      if ( !strcmp( pArea->filename, buf2 ) )
      {
        SPRINTF( buf, "%s%s", BUILD_DIR, buf2 );
        if ( IS_SET( pArea->status, AREA_LOADED ) )
          fold_area( pArea, buf, FALSE );
        close_area( pArea );
        SPRINTF( buf2, "%s.bak", buf );
        set_char_color( AT_RED, ch ); /* Log message changes colors */
        if ( !rename( buf, buf2 ) )
          send_to_char( "Player's area data destroyed.  Area saved as backup.\n", ch );
        else if ( errno != ENOENT )
        {
          ch_printf( ch, "Unknown error #%d - %s (area data).  Report to Thoric.\n",
                  errno, strerror( errno ) );
          SPRINTF( buf2, "%s destroying %s", ch->name, buf );
          perror( buf2 );
        }
      }
  }
  else if ( errno == ENOENT )
  {
    set_char_color( AT_PLAIN, ch );
    send_to_char( "Player does not exist.\n", ch );
  }
  else
  {
    set_char_color( AT_WHITE, ch );
    ch_printf( ch, "Unknown error #%d - %s.  Report to Thoric.\n",
            errno, strerror( errno ) );
    SPRINTF( buf, "%s destroying %s", ch->name, arg );
    perror( buf );
  }
  return;
}
extern ROOM_INDEX_DATA *       room_index_hash         [MAX_KEY_HASH]; /* db.c */


/* Super-AT command:

FOR ALL <action>
FOR MORTALS <action>
FOR GODS <action>
FOR MOBS <action>
FOR EVERYWHERE <action>


Executes action several times, either on ALL players (not including yourself),
MORTALS (including trusted characters), GODS (characters with level higher than
L_HERO), MOBS (Not recommended) or every room (not recommended either!)

If you insert a # in the action, it will be replaced by the name of the target.

If # is a part of the action, the action will be executed for every target
in game. If there is no #, the action will be executed for every room containg
at least one target, but only once per room. # cannot be used with FOR EVERY-
WHERE. # can be anywhere in the action.

Example: 

FOR ALL SMILE -> you will only smile once in a room with 2 players.
FOR ALL TWIDDLE # -> In a room with A and B, you will twiddle A then B.

Destroying the characters this command acts upon MAY cause it to fail. Try to
avoid something like FOR MOBS PURGE (although it actually works at my MUD).

FOR MOBS TRANS 3054 (transfer ALL the mobs to Midgaard temple) does NOT work
though :)

The command works by transporting the character to each of the rooms with 
target in them. Private rooms are not violated.

*/

/* Expand the name of a character into a string that identifies THAT
   character within a room. E.g. the second 'guard' -> 2. guard
*/   
const char * name_expand (CHAR_DATA *ch)
{
	int count = 1;
	CHAR_DATA *rch;
	char name[MAX_INPUT_LENGTH]; /*  HOPEFULLY no mob has a name longer than THAT */

	static char outbuf[MAX_INPUT_LENGTH];	
	
	if (!IS_NPC(ch))
		return ch->name;
		
	one_argument (ch->name, name); /* copy the first word into name */
	
	if (!name[0]) /* weird mob .. no keywords */
	{
		STRLCPY (outbuf, ""); /* Do not return NULL, just an empty buffer */
		return outbuf;
	}
	
	/* ->people changed to ->first_person -- TRI */	
	for (rch = ch->in_room->first_person; rch && (rch != ch);rch = 
	    rch->next_in_room)
		if (is_name (name, rch->name))
			count++;
			

	SPRINTF(outbuf, "%d.%s", count, name);
    return outbuf;
}


void do_for (CHAR_DATA *ch, char *argument)
{
	char range[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	bool fGods = FALSE, fMortals = FALSE, fMobs = FALSE, fEverywhere = FALSE, found;
	ROOM_INDEX_DATA *room, *old_room;
	CHAR_DATA *p, *p_prev;  /* p_next to p_prev -- TRI */
	int i;
	
	argument = one_argument (argument, range);
	
	if (!range[0] || !argument[0]) /* invalid usage? */
	{
		do_help (ch, "for");
		return;
	}
	
	if (!str_prefix("quit", argument))
	{
		send_to_char ("Are you trying to crash the MUD or something?\n",ch);
		return;
	}
	
	
	if (!str_cmp (range, "all"))
	{
		fMortals = TRUE;
		fGods = TRUE;
	}
	else if (!str_cmp (range, "gods"))
		fGods = TRUE;
	else if (!str_cmp (range, "mortals"))
		fMortals = TRUE;
	else if (!str_cmp (range, "mobs"))
		fMobs = TRUE;
	else if (!str_cmp (range, "everywhere"))
		fEverywhere = TRUE;
	else
		do_help (ch, "for"); /* show syntax */

	/* do not allow # to make it easier */		
	if (fEverywhere && strchr (argument, '#'))
	{
		send_to_char ("Cannot use FOR EVERYWHERE with the # thingie.\n",ch);
		return;
	}
		
	if (strchr (argument, '#')) /* replace # ? */
	{ 
		/* char_list - last_char, p_next - gch_prev -- TRI */
		for (p = last_char; p ; p = p_prev )
		{
			p_prev = p->prev;  /* TRI */
		/*	p_next = p->next; */ /* In case someone DOES try to AT MOBS SLAY # */
			found = FALSE;
			
			if (!(p->in_room) || room_is_private(p, p->in_room) || (p == ch))
				continue;
			
			if (IS_NPC(p) && fMobs)
				found = TRUE;
			else if (!IS_NPC(p) && get_trust(p) >= LEVEL_IMMORTAL && fGods)
				found = TRUE;
			else if (!IS_NPC(p) && get_trust(p) < LEVEL_IMMORTAL && fMortals)
				found = TRUE;

			/* It looks ugly to me.. but it works :) */				
			if (found) /* p is 'appropriate' */
			{
				char *pSource = argument; /* head of buffer to be parsed */
				char *pDest = buf; /* parse into this */
				
				while (*pSource)
				{
					if (*pSource == '#') /* Replace # with name of target */
					{
						const char *namebuf = name_expand (p);
						
						if (namebuf) /* in case there is no mob name ?? */
							while (*namebuf) /* copy name over */
								*(pDest++) = *(namebuf++);

						pSource++;
					}
					else
						*(pDest++) = *(pSource++);
				} /* while */
				*pDest = '\0'; /* Terminate */
				
				/* Execute */
				old_room = ch->in_room;
				char_from_room (ch);
				char_to_room (ch,p->in_room);
				interpret (ch, buf);
				char_from_room (ch);
				char_to_room (ch,old_room);
				
			} /* if found */
		} /* for every char */
	}
	else /* just for every room with the appropriate people in it */
	{
		for (i = 0; i < MAX_KEY_HASH; i++) /* run through all the buckets */
			for (room = room_index_hash[i] ; room ; room = room->next)
			{
				found = FALSE;
				
				/* Anyone in here at all? */
				if (fEverywhere) /* Everywhere executes always */
					found = TRUE;
				else if (!room->first_person) /* Skip it if room is empty */
					continue;
				/* ->people changed to first_person -- TRI */
					
				/* Check if there is anyone here of the requried type */
				/* Stop as soon as a match is found or there are no more ppl in room */
				/* ->people to ->first_person -- TRI */
				for (p = room->first_person; p && !found; p = p->next_in_room)
				{

					if (p == ch) /* do not execute on oneself */
						continue;
						
					if (IS_NPC(p) && fMobs)
						found = TRUE;
					else if (!IS_NPC(p) && ( get_trust(p) >= LEVEL_IMMORTAL) && fGods)
						found = TRUE;
					else if (!IS_NPC(p) && ( get_trust(p) <= LEVEL_IMMORTAL) && fMortals)
						found = TRUE;
				} /* for everyone inside the room */
						
				if (found && !room_is_private(p, room)) /* Any of the required type here AND room not private? */
				{
					/* This may be ineffective. Consider moving character out of old_room
					   once at beginning of command then moving back at the end.
					   This however, is more safe?
					*/
				
					old_room = ch->in_room;
					char_from_room (ch);
					char_to_room (ch, room);
					interpret (ch, argument);
					char_from_room (ch);
					char_to_room (ch, old_room);
				} /* if found */
			} /* for every room in a bucket */
	} /* if strchr */
} /* do_for */

void save_sysdata  args( ( SYSTEM_DATA sys ) );

void do_cset( CHAR_DATA *ch, char *argument )
{
  char arg[MAX_STRING_LENGTH];
  sh_int level;

  set_char_color( AT_IMMORT, ch );

  if (argument[0] == '\0')
  {
    ch_printf(ch, "Mail:\n  Read all mail: %d. Read mail for free: %d. Write mail for free: %d.\n",
	    sysdata.read_all_mail, sysdata.read_mail_free, sysdata.write_mail_free );
    ch_printf(ch, "  Take all mail: %d.\n",
	    sysdata.take_others_mail);
    ch_printf(ch, "Channels:\n  Muse: %d. Think: %d. Log: %d. Build: %d.\n",
 	    sysdata.muse_level, sysdata.think_level, sysdata.log_level, 
	    sysdata.build_level);
    ch_printf(ch, "Building:\n  Prototype modification: %d.  Player msetting: %d.\n",
	    sysdata.level_modify_proto, sysdata.level_mset_player );
    ch_printf(ch, "Guilds:\n  Overseer: %s.  Advisor: %s.\n", 
            sysdata.guild_overseer, sysdata.guild_advisor );
    ch_printf(ch, "Other:\n  Force on players: %d.  ", sysdata.level_forcepc);
    ch_printf(ch, "Private room override: %d.\n", sysdata.level_override_private);
    ch_printf(ch, "  Penalty to regular stun chance: %d.  ", sysdata.stun_regular );
    ch_printf(ch, "Penalty to stun plr vs. plr: %d.\n", sysdata.stun_plr_vs_plr );
    ch_printf(ch, "  Percent damage plr vs. plr: %3d.  ", sysdata.dam_plr_vs_plr );
    ch_printf(ch, "Percent damage plr vs. mob: %d.\n", sysdata.dam_plr_vs_mob );
    ch_printf(ch, "  Percent damage mob vs. plr: %3d.  ", sysdata.dam_mob_vs_plr );
    ch_printf(ch, "Percent damage mob vs. mob: %d.\n", sysdata.dam_mob_vs_mob );
    ch_printf(ch, "  Get object without take flag: %d.  ", sysdata.level_getobjnotake);
    ch_printf(ch, "Autosave frequency (minutes): %d.\n", sysdata.save_frequency );
    ch_printf(ch, "  Save flags: %s\n", flag_string( sysdata.save_flags, save_flag ) );
    return;
  }

  argument = one_argument( argument, arg );

  if (!str_cmp(arg, "help"))
  {
     do_help(ch, "controls");
     return;
  }

  if (!str_cmp(arg, "save"))
  {
     save_sysdata(sysdata);
     return;
  }

  if (!str_cmp(arg, "saveflag"))
  {
	int x = get_saveflag( argument );

	if ( x == -1 )
	    send_to_char( "Not a save flag.\n", ch );
	else
	{
	    TOGGLE_BIT( sysdata.save_flags, 1 << x );
	    send_to_char( "Ok.\n", ch );
	}
	return;
  }

  if (!str_prefix( arg, "guild_overseer" ) )
  {
    STRFREE( sysdata.guild_overseer );
    sysdata.guild_overseer = str_dup( argument );
    send_to_char("Ok.\n", ch);      
    return;
  }
  if (!str_prefix( arg, "guild_advisor" ) )
  {
    STRFREE( sysdata.guild_advisor );
    sysdata.guild_advisor = str_dup( argument );
    send_to_char("Ok.\n", ch);      
    return;
  }

  level = (sh_int) atoi(argument);

  if (!str_prefix( arg, "savefrequency" ) )
  {
    sysdata.save_frequency = level;
    send_to_char("Ok.\n", ch);
    return;
  }

  if (!str_cmp(arg, "stun"))
  {
    sysdata.stun_regular = level;
    send_to_char("Ok.\n", ch);      
    return;
  }

  if (!str_cmp(arg, "stun_pvp"))
  {
    sysdata.stun_plr_vs_plr = level;
    send_to_char("Ok.\n", ch);      
    return;
  }

  if (!str_cmp(arg, "dam_pvp"))
  {
    sysdata.dam_plr_vs_plr = level;
    send_to_char("Ok.\n", ch);      
    return;
  }

  if (!str_cmp(arg, "get_notake"))
  {
    sysdata.level_getobjnotake = level;
    send_to_char("Ok.\n", ch);      
    return;
  }

  if (!str_cmp(arg, "dam_pvm"))
  {
    sysdata.dam_plr_vs_mob = level;
    send_to_char("Ok.\n", ch);      
    return;
  }

  if (!str_cmp(arg, "dam_mvp"))
  {
    sysdata.dam_mob_vs_plr = level;
    send_to_char("Ok.\n", ch);      
    return;
  }

  if (!str_cmp(arg, "dam_mvm"))
  {
    sysdata.dam_mob_vs_mob = level;
    send_to_char("Ok.\n", ch);      
    return;
  }

  if (level < 0 || level > MAX_LEVEL)
  {
    send_to_char("Invalid value for new control.\n", ch);
    return;
  }

  if (!str_cmp(arg, "read_all"))
  {
    sysdata.read_all_mail = level;
    send_to_char("Ok.\n", ch);      
    return;
  }

  if (!str_cmp(arg, "read_free"))
  {
    sysdata.read_mail_free = level;
    send_to_char("Ok.\n", ch);      
    return;
  }
  if (!str_cmp(arg, "write_free"))
  {
    sysdata.write_mail_free = level;
    send_to_char("Ok.\n", ch);      
    return;
  }
  if (!str_cmp(arg, "take_all"))
  {
    sysdata.take_others_mail = level;
    send_to_char("Ok.\n", ch);      
    return;
  }
  if (!str_cmp(arg, "muse"))
  {
    sysdata.muse_level = level;
    send_to_char("Ok.\n", ch);      
    return;
  }
  if (!str_cmp(arg, "think"))
  {
    sysdata.think_level = level;
    send_to_char("Ok.\n", ch);      
    return;
  }
  if (!str_cmp(arg, "log"))
  {
    sysdata.log_level = level;
    send_to_char("Ok.\n", ch);      
    return;
  }
  if (!str_cmp(arg, "build"))
  {
    sysdata.build_level = level;
    send_to_char("Ok.\n", ch);      
    return;
  }
  if (!str_cmp(arg, "proto_modify"))
  {
    sysdata.level_modify_proto = level;
    send_to_char("Ok.\n", ch);      
    return;
  }
  if (!str_cmp(arg, "override_private"))
  {
    sysdata.level_override_private = level;
    send_to_char("Ok.\n", ch);      
    return;
  }
  if (!str_cmp(arg, "forcepc"))
  {
    sysdata.level_forcepc = level;
    send_to_char("Ok.\n", ch);      
    return;
  }
  if (!str_cmp(arg, "mset_player"))
  {
    sysdata.level_mset_player = level;
    send_to_char("Ok.\n", ch);      
    return;
  }
  else
  {
    send_to_char("Invalid argument.\n", ch);
    return;
  }
}

void get_reboot_string(void)
{
  SPRINTF(reboot_time, "%s", asctime(new_boot_time));
}


void do_orange( CHAR_DATA *ch, char *argument )
{
  send_to_char( "Function under construction.\n", ch );
  return;
}

void do_mrange( CHAR_DATA *ch, char *argument )
{
  send_to_char( "Function under construction.\n", ch );
  return;
}

void do_hell( CHAR_DATA *ch, char *argument )
{
  CHAR_DATA *victim;
  char arg[MAX_INPUT_LENGTH];
  sh_int time;
  bool h_d = FALSE;
  struct tm *tms;
  
  argument = one_argument(argument, arg);
  if ( !*arg )
  {
    send_to_char( "Hell who, and for how long?\n", ch );
    return;
  }
  if ( !(victim = get_char_world(ch, arg)) || IS_NPC(victim) )
  {
    send_to_char( "They aren't here.\n", ch );
    return;
  }
  if ( IS_IMMORTAL(victim) )
  {
    send_to_char( "There is no point in helling an immortal.\n", ch );
    return;
  }
  if ( victim->pcdata->release_date != 0 )
  {
    ch_printf(ch, "They are already in hell until %24.24s, by %s.\n",
            ctime(&victim->pcdata->release_date), victim->pcdata->helled_by);
    return;
  }
  argument = one_argument(argument, arg);
  if ( !*arg || !is_number(arg) )
  {
    send_to_char( "Hell them for how long?\n", ch );
    return;
  }
  time = atoi(arg);
  if ( time <= 0 )
  {
    send_to_char( "You cannot hell for zero or negative time.\n", ch );
    return;
  }
  argument = one_argument(argument, arg);
  if ( !*arg || !str_prefix(arg, "hours") )
    h_d = TRUE;
  else if ( str_prefix(arg, "days") )
  {
    send_to_char( "Is that value in hours or days?\n", ch );
    return;
  }
  else if ( time > 30 )
  {
    send_to_char( "You may not hell a person for more than 30 days at a time.\n", ch );
    return;
  }
  tms = localtime(&current_time);
  if ( h_d )
    tms->tm_hour += time;
  else
    tms->tm_mday += time;
  victim->pcdata->release_date = mktime(tms);
  victim->pcdata->helled_by = STRALLOC(ch->name);
  ch_printf(ch, "%s will be released from hell at %24.24s.\n", victim->name,
          ctime(&victim->pcdata->release_date));
  act(AT_MAGIC, "$n disappears in a cloud of hellish light.", victim, NULL, ch, TO_NOTVICT);
  char_from_room(victim);
  char_to_room(victim, get_room_index(6));
  act(AT_MAGIC, "$n appears in a could of hellish light.", victim, NULL, ch, TO_NOTVICT);
  do_look(victim, "auto");
  ch_printf(victim, "The immortals are not pleased with your actions.\n"
          "You shall remain in hell for %d %s%s.\n", time,
          (h_d ? "hour" : "day"), (time == 1 ? "" : "s"));
  save_char_obj(victim);	/* used to save ch, fixed by Thoric 09/17/96 */
  return;
}

void do_unhell( CHAR_DATA *ch, char *argument )
{
  CHAR_DATA *victim;
  char arg[MAX_INPUT_LENGTH];
  ROOM_INDEX_DATA *location;
  
  argument = one_argument(argument, arg);
  if ( !*arg )
  {
    send_to_char( "Unhell whom..?\n", ch );
    return;
  }
  location = ch->in_room;
  ch->in_room = get_room_index(6);
  victim = get_char_room(ch, arg);
  ch->in_room = location;            /* The case of unhell self, etc. */
  if ( !victim || IS_NPC(victim) || victim->in_room->vnum != 6 )
  {
    send_to_char( "No one like that is in hell.\n", ch );
    return;
  }
  location = get_room_index( wherehome(victim) );
  if ( !location )
    location = ch->in_room;
  MOBtrigger = FALSE;
  act( AT_MAGIC, "$n disappears in a cloud of godly light.", victim, NULL, ch, TO_NOTVICT );
  char_from_room(victim);
  char_to_room(victim, location);
  send_to_char( "The gods have smiled on you and released you from hell early!\n", victim );
  do_look(victim, "auto");
  send_to_char( "They have been released.\n", ch );

  if ( victim->pcdata->helled_by )
  {
    if( str_cmp(ch->name, victim->pcdata->helled_by) )
      ch_printf(ch, "(You should probably write a note to %s, explaining the early release.)\n",
            victim->pcdata->helled_by);
    STRFREE(victim->pcdata->helled_by);
    victim->pcdata->helled_by = NULL;
  }

  MOBtrigger = FALSE;
  act( AT_MAGIC, "$n appears in a cloud of godly light.", victim, NULL, ch, TO_NOTVICT );
  victim->pcdata->release_date = 0;
  save_char_obj(victim);
  return;
}

/* Vnum search command by Swordbearer */
void do_vsearch( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    bool found = FALSE;
    OBJ_DATA *obj;
    OBJ_DATA *in_obj;
    int obj_counter = 1;
    int argi;
 
    one_argument( argument, arg );
 
    if( arg[0] == '\0' )
    {
        send_to_char( "Syntax:  vsearch <vnum>.\n", ch );
        return;
    }
 
    set_pager_color( AT_PLAIN, ch );
    argi=atoi(arg);
    if (argi<0 || argi>20000)
    {
	send_to_char( "Vnum out of range.\n", ch);
	return;
    }
    for ( obj = first_object; obj != NULL; obj = obj->next )
    {
	if ( !can_see_obj( ch, obj ) || !( argi == obj->pIndexData->vnum ))
	  continue;
 
	found = TRUE;
	for ( in_obj = obj; in_obj->in_obj != NULL;
	  in_obj = in_obj->in_obj );

	if ( in_obj->carried_by != NULL )
	  pager_printf( ch, "[%2d] Level %d %s carried by %s.\n", 
		obj_counter,
		obj->level, obj_short(obj),
		PERS( in_obj->carried_by, ch ) );
	else           
	  pager_printf( ch, "[%2d] [%-5d] %s in %s.\n", obj_counter,
		( ( in_obj->in_room ) ? in_obj->in_room->vnum : 0 ),
		obj_short(obj), ( in_obj->in_room == NULL ) ?
		"somewhere" : in_obj->in_room->name );

	obj_counter++;
    }

    if ( !found )
      send_to_char( "Nothing like that in hell, earth, or heaven.\n" , ch );

    return;
}

/* 
 * Simple function to let any imm make any player instantly sober.
 * Saw no need for level restrictions on this.
 * Written by Narn, Apr/96 
 */
void do_sober( CHAR_DATA *ch, char *argument )
{
  CHAR_DATA *victim;
  char arg1 [MAX_INPUT_LENGTH];

  smash_tilde( argument );
  argument = one_argument( argument, arg1 );
  if ( ( victim = get_char_room( ch, arg1 ) ) == NULL )
  {
    send_to_char( "They aren't here.\n", ch );
    return;
  }

  if ( IS_NPC( victim ) )
  {
    send_to_char( "Not on mobs.\n", ch );
    return;    
  }

  if ( victim->pcdata ) 
    victim->pcdata->condition[COND_DRUNK] = 0;
  send_to_char( "Ok.\n", ch );
  send_to_char( "You feel sober again.\n", victim );
  return;    
}


/*
 * Free a social structure					-Thoric
 */
void free_social( SOCIALTYPE *social )
{
    if ( social->name )
      STR_DISPOSE( social->name );
    if ( social->char_no_arg )
      STR_DISPOSE( social->char_no_arg );
    if ( social->others_no_arg )
      STR_DISPOSE( social->others_no_arg );
    if ( social->char_found )
      STR_DISPOSE( social->char_found );
    if ( social->others_found )
      STR_DISPOSE( social->others_found );
    if ( social->vict_found )
      STR_DISPOSE( social->vict_found );
    if ( social->char_auto )
      STR_DISPOSE( social->char_auto );
    if ( social->others_auto )
      STR_DISPOSE( social->others_auto );
    DISPOSE( social );
}

/*
 * Remove a social from it's hash index				-Thoric
 */
void unlink_social( SOCIALTYPE *social )
{
    SOCIALTYPE *tmp, *tmp_next;
    int hash;

    if ( !social )
    {
        bug( "%s: NULL social", __func__ );
	    return;
    }

    if ( social->name[0] < 'a' || social->name[0] > 'z' )
	hash = 0;
    else
	hash = (social->name[0] - 'a') + 1;

    if ( social == (tmp=social_index[hash]) )
    {
	social_index[hash] = tmp->next;
	return;
    }
    for ( ; tmp; tmp = tmp_next )
    {
	tmp_next = tmp->next;
	if ( social == tmp_next )
	{
	    tmp->next = tmp_next->next;
	    return;
	}
    }
}

/*
 * Add a social to the social index table			-Thoric
 * Hashed and insert sorted
 */
void add_social( SOCIALTYPE *social )
{
    int hash, x;
    SOCIALTYPE *tmp, *prev;

    if ( !social )
    {
        bug( "%s: NULL social", __func__ );
	    return;
    }

    if ( !social->name )
    {
        bug( "%s: NULL social->name", __func__ );
	    return;
    }

    if ( !social->char_no_arg )
    {
        bug( "%s: NULL social->char_no_arg", __func__ );
	    return;
    }

    /* make sure the name is all lowercase */
    for ( x = 0; social->name[x] != '\0'; x++ )
	social->name[x] = LOWER(social->name[x]);

    if ( social->name[0] < 'a' || social->name[0] > 'z' )
	hash = 0;
    else
	hash = (social->name[0] - 'a') + 1;

    if ( (prev = tmp = social_index[hash]) == NULL )
    {
	social->next = social_index[hash];
	social_index[hash] = social;
	return;
    }

    for ( ; tmp; tmp = tmp->next )
    {
	if ( (x=strcmp(social->name, tmp->name)) == 0 )
	{
        bug( "%s: trying to add duplicate name to bucket %d", __func__, hash );
	    free_social( social );
	    return;
	}
	else
	if ( x < 0 )
	{
	    if ( tmp == social_index[hash] )
	    {
		social->next = social_index[hash];
		social_index[hash] = social;
		return;
	    }
	    prev->next = social;
	    social->next = tmp;
	    return;
	}
	prev = tmp;
    }

    /* add to end */
    prev->next = social;
    social->next = NULL;
    return;
}

/*
 * Social editor/displayer/save/delete				-Thoric
 */
void do_sedit( CHAR_DATA *ch, char *argument )
{
    SOCIALTYPE *social;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];

    smash_tilde( argument );
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    set_char_color( AT_SOCIAL, ch );

    if ( arg1[0] == '\0' )
    {
	send_to_char( "Syntax: sedit <social> [field]\n", ch );
	send_to_char( "Syntax: sedit <social> create\n", ch );
	if ( get_trust(ch) > LEVEL_GOD )
	    send_to_char( "Syntax: sedit <social> delete\n", ch );
	if ( get_trust(ch) > LEVEL_LESSER )
	    send_to_char( "Syntax: sedit <save>\n", ch );
	send_to_char( "\nField being one of:\n", ch );
	send_to_char( "  cnoarg onoarg cfound ofound vfound cauto oauto\n", ch );
	return;
    }

    if ( get_trust(ch) > LEVEL_LESSER && !str_cmp( arg1, "save" ) )
    {
	save_socials();
	send_to_char( "Saved.\n", ch );
	return;
    }

    social = find_social( arg1 );

    if ( !str_cmp( arg2, "create" ) )
    {
	if ( social )
	{
	    send_to_char( "That social already exists!\n", ch );
	    return;
	}
	CREATE( social, SOCIALTYPE, 1 );
	social->name = str_dup( arg1 );
	SPRINTF( arg2, "You %s.", arg1 );

   social->char_no_arg = str_dup( arg2 );
	add_social( social );
	send_to_char( "Social added.\n", ch );
	return;
    }

    if ( !social )
    {
	send_to_char( "Social not found.\n", ch );
	return;
    }

    if ( arg2[0] == '\0' || !str_cmp( arg2, "show" ) )
    {
	ch_printf( ch, "Social: %s\n\nCNoArg: %s\n",
	    social->name,	social->char_no_arg );
	ch_printf( ch, "ONoArg: %s\nCFound: %s\nOFound: %s\n",
	    social->others_no_arg	? social->others_no_arg	: "(not set)",
	    social->char_found		? social->char_found	: "(not set)",
	    social->others_found	? social->others_found	: "(not set)" );
	ch_printf( ch, "VFound: %s\nCAuto : %s\nOAuto : %s\n",
	    social->vict_found	? social->vict_found	: "(not set)",
	    social->char_auto	? social->char_auto	: "(not set)",
	    social->others_auto	? social->others_auto	: "(not set)" );
	return;
    }

    if ( get_trust(ch) > LEVEL_GOD && !str_cmp( arg2, "delete" ) )
    {
	unlink_social( social );
	free_social( social );
	send_to_char( "Deleted.\n", ch );
	return;
    }

    if ( !str_cmp( arg2, "cnoarg" ) )
    {
	if ( argument[0] == '\0' || !str_cmp( argument, "clear" ) )
	{
	    send_to_char( "You cannot clear this field.  It must have a message.\n", ch );
	    return;
	}
	if ( social->char_no_arg )
	    STR_DISPOSE( social->char_no_arg );
	social->char_no_arg = str_dup( argument );
	send_to_char( "Done.\n", ch );
	return;
    }

    if ( !str_cmp( arg2, "onoarg" ) )
    {
	if ( social->others_no_arg )
	    STR_DISPOSE( social->others_no_arg );
	if ( argument[0] != '\0' && str_cmp( argument, "clear" ) )
	    social->others_no_arg = str_dup( argument );
	send_to_char( "Done.\n", ch );
	return;
    }

    if ( !str_cmp( arg2, "cfound" ) )
    {
	if ( social->char_found )
	    STR_DISPOSE( social->char_found );
	if ( argument[0] != '\0' && str_cmp( argument, "clear" ) )
	    social->char_found = str_dup( argument );
	send_to_char( "Done.\n", ch );
	return;
    }

    if ( !str_cmp( arg2, "ofound" ) )
    {
	if ( social->others_found )
	    STR_DISPOSE( social->others_found );
	if ( argument[0] != '\0' && str_cmp( argument, "clear" ) )
	    social->others_found = str_dup( argument );
	send_to_char( "Done.\n", ch );
	return;
    }

    if ( !str_cmp( arg2, "vfound" ) )
    {
	if ( social->vict_found )
	    STR_DISPOSE( social->vict_found );
	if ( argument[0] != '\0' && str_cmp( argument, "clear" ) )
	    social->vict_found = str_dup( argument );
	send_to_char( "Done.\n", ch );
	return;
    }

    if ( !str_cmp( arg2, "cauto" ) )
    {
	if ( social->char_auto )
	    STR_DISPOSE( social->char_auto );
	if ( argument[0] != '\0' && str_cmp( argument, "clear" ) )
	    social->char_auto = str_dup( argument );
	send_to_char( "Done.\n", ch );
	return;
    }

    if ( !str_cmp( arg2, "oauto" ) )
    {
	if ( social->others_auto )
	    STR_DISPOSE( social->others_auto );
	if ( argument[0] != '\0' && str_cmp( argument, "clear" ) )
	    social->others_auto = str_dup( argument );
	send_to_char( "Done.\n", ch );
	return;
    }

    if ( get_trust(ch) > LEVEL_GREATER && !str_cmp( arg2, "name" ) )
    {
        bool relocate;
        SOCIALTYPE *checksocial;    

        one_argument( argument, arg1 );
        if ( arg1[0] == '\0' )
        {
            send_to_char( "Cannot clear name field!\n", ch );
            return;
        }
        if( ( checksocial = find_social( arg1 ) ) != NULL )
        {
            ch_printf( ch, "There is already a social named %s.\n", arg1 );
            return;
        }        
        if ( arg1[0] != social->name[0] )
        {
            unlink_social( social );
            relocate = TRUE;
        }
        else
            relocate = FALSE;
        if ( social->name )
            STR_DISPOSE( social->name );
        social->name = str_dup( arg1 );
        if ( relocate )
            add_social( social );
        send_to_char( "Done.\n", ch );
        return;
    }

    /* display usage message */
    do_sedit( ch, "" );
}

/*
 * Free a command structure					-Thoric
 */
void free_command( CMDTYPE *command )
{
    if ( command->name )
      STR_DISPOSE( command->name );
    DISPOSE( command );
}

/*
 * Remove a command from it's hash index			-Thoric
 */
void unlink_command( CMDTYPE *command )
{
    CMDTYPE *tmp, *tmp_next;
    int hash;

    if ( !command )
    {
        bug( "%s: NULL command", __func__ );
	    return;
    }

    hash = command->name[0]%126;

    if ( command == (tmp=command_hash[hash]) )
    {
	command_hash[hash] = tmp->next;
	return;
    }
    for ( ; tmp; tmp = tmp_next )
    {
	tmp_next = tmp->next;
	if ( command == tmp_next )
	{
	    tmp->next = tmp_next->next;
	    return;
	}
    }
}

/*
 * Add a command to the command hash table			-Thoric
 */
void add_command( CMDTYPE *command )
{
    int hash, x;
    CMDTYPE *tmp, *prev;

    if ( !command )
    {
        bug( "%s: NULL command", __func__ );
	    return;
    }

    if ( !command->name )
    {
        bug( "%s: NULL command->name", __func__ );
	    return;
    }

    if ( !command->do_fun )
    {
        bug( "%s: NULL command->do_fun", __func__ );
	    return;
    }

    /* make sure the name is all lowercase */
    for ( x = 0; command->name[x] != '\0'; x++ )
	command->name[x] = LOWER(command->name[x]);

    hash = command->name[0] % 126;

    if ( (prev = tmp = command_hash[hash]) == NULL )
    {
	command->next = command_hash[hash];
	command_hash[hash] = command;
	return;
    }

    /* add to the END of the list */
    for ( ; tmp; tmp = tmp->next )
	if ( !tmp->next )
	{
	    tmp->next = command;
	    command->next = NULL;
	}
    return;
}

/*
 * Command editor/displayer/save/delete				-Thoric
 */
void do_cedit( CHAR_DATA *ch, char *argument )
{
    CMDTYPE *command;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char buf[128];

    smash_tilde( argument );
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    set_char_color( AT_IMMORT, ch );

    if ( arg1[0] == '\0' )
    {
	send_to_char( "Syntax: cedit save\n", ch );
	if ( get_trust(ch) > LEVEL_SUB_IMPLEM )
	{
	    send_to_char( "Syntax: cedit <command> create [code]\n", ch );
	    send_to_char( "Syntax: cedit <command> delete\n", ch );
	    send_to_char( "Syntax: cedit <command> show\n", ch );
	    send_to_char( "Syntax: cedit <command> [field]\n", ch );
	    send_to_char( "\nField being one of:\n", ch );
	    send_to_char( "  level position log code commandgroup\n", ch );
	}
	return;
    }

    if ( get_trust(ch) > LEVEL_GREATER && !str_cmp( arg1, "save" ) )
    {
	save_commands();
	send_to_char( "Saved.\n", ch );
	return;
    }

    command = find_command( arg1 );

    if ( get_trust(ch) > LEVEL_SUB_IMPLEM && !str_cmp( arg2, "create" ) )
    {
	if ( command )
	{
	    send_to_char( "That command already exists!\n", ch );
	    return;
	}
	CREATE( command, CMDTYPE, 1 );
	command->name = str_dup( arg1 );
	command->level = get_trust(ch);
	if ( *argument )
	  one_argument(argument, arg2);
	else
	{
	    SPRINTF( arg2, "do_%s", arg1 );
	}
	command->do_fun = skill_function( arg2 );
	add_command( command );
	send_to_char( "Command added.\n", ch );
	if ( command->do_fun == skill_notfound )
	  ch_printf( ch, "Code %s not found.  Set to no code.\n", arg2 );
	return;
    }

    if ( !command )
    {
	send_to_char( "Command not found.\n", ch );
	return;
    }
    else
    if ( command->level > get_trust(ch) )
    {
	send_to_char( "You cannot touch this command.\n", ch );
	return;
    }

    if ( arg2[0] == '\0' || !str_cmp( arg2, "show" ) )
    {
        int i;
	ch_printf( ch, "Command:  %s\nLevel:    %d\nPosition: %d\nLog:      %d\nCode:     %s\n",
	    command->name, command->level, command->position, command->log,
	    skill_name(command->do_fun) );
	if ( command->userec.num_uses )
	  send_timer(&command->userec, ch);
        ch_printf(ch, "Command Groups: ");
        for(i = 0; i < MAX_COMMAND_GROUP; i++)
        {
           if (BV_IS_SET(command->commandgroup,(i)))
             ch_printf(ch,"%s ",get_flag_name(command_groups, i, MAX_COMMAND_GROUP));
        }
        ch_printf(ch,"\n");
	return;
    }

    if ( get_trust(ch) <= LEVEL_SUB_IMPLEM )
    {
	do_cedit( ch, "" );
	return;
    }

    if ( !str_cmp( arg2, "delete" ) )
    {
	unlink_command( command );
	free_command( command );
	send_to_char( "Deleted.\n", ch );
	return;
    }

    if ( !str_cmp( arg2, "code" ) )
    {
	DO_FUN *fun = skill_function( argument );
	
	if ( fun == skill_notfound )
	{
	    send_to_char( "Code not found.\n", ch );
	    return;
	}
	command->do_fun = fun;
	send_to_char( "Done.\n", ch );
	return;
    }

    if ( !str_cmp( arg2, "commandgroup" ) )
    {
        int i;
        int grp = atoi(argument);

        if (grp < 0 || grp > MAX_COMMAND_GROUP )
        {
            send_to_char("Command Groups: \n",ch);
            for (i = 0; i <  MAX_COMMAND_GROUP; i++) {
              SPRINTF(buf,"%d) %s\n",i, get_flag_name(command_groups, i, MAX_COMMAND_GROUP));
              send_to_char(buf,ch);
            }
            return;
        }
        BV_TOGGLE_BIT(command->commandgroup,(grp));
        send_to_char( "Done.\n", ch );
        return;
    }


    if( !str_cmp( arg2, "level" ) )
    {
      int level = atoi( argument );

      if( ( level < 0 || level > get_trust( ch ) ) )
      {
         send_to_char( "Level out of range.\n", ch );
         return;
      }

      if ( level > command->level && command->do_fun == do_switch )
      {
         command->level = level;
         check_switches(FALSE);
      }
      else
         command->level = level;
      send_to_char( "Done.\n", ch );
      return;
    }

    if ( !str_cmp( arg2, "log" ) )
    {
	int log = atoi( argument );

	if ( log < 0 || log > LOG_COMM )
	{
	    send_to_char( "Log out of range.\n", ch );
	    return;
	}
	command->log = log;
	send_to_char( "Done.\n", ch );
	return;
    }

    if ( !str_cmp( arg2, "position" ) )
    {
	int position = atoi( argument );

	if ( position < 0 || position > POS_DRAG )
	{
	    send_to_char( "Position out of range.\n", ch );
	    return;
	}
	command->position = position;
	send_to_char( "Done.\n", ch );
	return;
    }

    if ( !str_cmp( arg2, "name" ) )
    {
    	bool relocate;
          CMDTYPE *checkcmd;    

        one_argument( argument, arg1 );
        if ( arg1[0] == '\0' )
        {
            send_to_char( "Cannot clear name field!\n", ch );
            return;
        }
        if( ( checkcmd = find_command( arg1 ) ) != NULL )
        {
            ch_printf( ch, "There is already a command named %s.\n", arg1 );
            return;
        }        
        if ( arg1[0] != command->name[0] )
        {
            unlink_command( command );
            relocate = TRUE;
        }
        else
            relocate = FALSE;
        if ( command->name )
            STR_DISPOSE( command->name );
        command->name = str_dup( arg1 );
        if ( relocate )
            add_command( command );
        send_to_char( "Done.\n", ch );
        return;
    }

    /* display usage message */
    do_cedit( ch, "" );
}
void do_badname(CHAR_DATA *ch, char *arguments)
{
  if (arguments[0] == '\0')
  {
    send_to_char("Usage: badname <name>\n",ch);
    return;
  }
  switch (add_bad_name(arguments))
  {
    case -1 : send_to_char("Error opening badname file.\n",ch); break;
    case  0 : send_to_char("That name is already in the badname file.\n",ch); break;
    case  1 : send_to_char("Name successfully added to the badname file.\n",ch); break;
    default : send_to_char("If you're reading this, add_bad_name is really messed up.\n",ch);break;
  }
  return;
}


void do_dnd( CHAR_DATA *ch, char *argument )
{
   if ( !IS_NPC(ch) && ch->pcdata )
      if ( BV_IS_SET(ch->pcdata->flags, PCFLAG_DND) )
      {
          BV_REMOVE_BIT(ch->pcdata->flags, PCFLAG_DND);
          send_to_char( "Your 'do not disturb' flag is now off.\n", ch );
      }
      else
      {
          BV_SET_BIT(ch->pcdata->flags, PCFLAG_DND);
          send_to_char( "Your 'do not disturb' flag is now on.\n", ch );
      }
   else
   send_to_char( "huh?\n", ch );
}

 void do_dontresolve(CHAR_DATA *ch, char *arguments)
 {
   if (arguments[0] == '\0')
   {
     send_to_char("Usage: dontresolve <IP match>\n",ch);
     return;
   }
   switch (add_dont_resolve(arguments))
   {
     case -1 : send_to_char("Error opening dontresolve file.\n",ch); break;
     case  0 : send_to_char("That IP match is already in the dontresolve file.\n",ch); break;
     case  1 : send_to_char("IP match successfully added to the dontresolve file.\n",ch); break;
     default : send_to_char("If you're reading this, add_dont_resolve is really messed up.\n",ch);break;
   }
   return;
 }

void do_viewskills( CHAR_DATA *ch, char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   char buf[MAX_STRING_LENGTH];
   CHAR_DATA *victim;
   int sn;
   int col;

    argument = one_argument( argument, arg );
    if ( arg[0] == '\0' )
   {
	send_to_char( "&zSyntax: skills <player>.\n", ch );
	return;
   }
   if ( ( victim = get_char_world( ch, arg ) ) == NULL )
   {
	send_to_char("No such person in the game.\n", ch );
	return;
   }

   col = 0;

   if ( !IS_NPC( victim ) )
   {
     set_char_color( AT_MAGIC, ch );
     for ( sn = 0; sn < top_sn && skill_table[sn] && skill_table[sn]->name; sn++ )
     {
	if ( skill_table[sn]->name == NULL )
	    break;
	if ( victim->pcdata->learned[sn] == 0 )
	    continue;

    SPRINTF( buf, "%20s %3d%% ", skill_table[sn]->name,
    victim->pcdata->learned[sn]);
	send_to_char( buf, ch );

	    if ( ++col % 3 == 0 )
		send_to_char( "\n", ch );
    }
  }
 return;
}

void do_undead( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    ROOM_INDEX_DATA *in_room;
    DESCRIPTOR_DATA *d;


    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg2 );

    if (arg[0] == '\0' || arg2[0] =='\0') {
      send_to_char("Usage: undead <dead char> <current char>\n",ch);
      return;
    }

    if ( (( victim = get_char_world( ch, arg2 ) ) == NULL) || IS_NPC(victim))
     {
        send_to_char("No such person in the game.\n", ch );
        return;
     }


    d = victim->desc;
    in_room = victim->in_room;

    if (load_char_obj_v2(d,arg,0,1)) {
/*	 //Copy prev and next pointers	
	 if (victim->prev)
           victim->prev->next = d->character;
         d->character->next = victim->next;
         if (victim->next != NULL)
           victim->next->prev = d->character;

	 //Clear out important parts of old char
	 victim->desc = NULL;
	 victim->prev = NULL;
	 victim->next = NULL;*/
	 victim->desc = NULL;
         extract_char(victim,TRUE);

	 //Set new parts
         victim = d->character;
         victim->hit = victim->max_hit;
         gmcp_evt_char_vitals(victim);
         victim->position = POS_STANDING;
	 add_char(victim);
         char_to_room(victim, in_room);
         if (victim->pcdata->clan) {
                update_member(victim);
         }
        send_to_char("You have been restored.\n",victim);
        send_to_char("Player was restored.\n",ch);
    } else {
        send_to_char("You were not able to be restored.\n",victim);
        send_to_char("Player was unable to be restored.\n",ch);
    }
    return;
}
