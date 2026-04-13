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
*			     Special clan module			    *
****************************************************************************/

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
/* #include <stdlib.h> */
#include <time.h>
#include "mud.h"

#define MAX_NEST	100

CLAN_DATA * first_clan;
CLAN_DATA * last_clan;

MEMBER_LIST * first_member_list;
MEMBER_LIST * last_member_list;

SENATE_DATA * first_senator;
SENATE_DATA * last_senator;

PLANET_DATA * first_planet;
PLANET_DATA * last_planet;

GUARD_DATA * first_guard;
GUARD_DATA * last_guard;

/* local routines */
void	fread_clan	args( ( CLAN_DATA *clan, FILE *fp ) );
bool	load_clan_file	args( ( GameContext *game, const std::string& clanfile ) );
void	write_clan_list	args( ( void ) );
void	fread_planet	args( ( PLANET_DATA *planet, FILE *fp ) );
bool	load_planet_file	args( ( GameContext *game, const std::string& planetfile ) );
void	write_planet_list	args( ( void ) );
void	save_member_list	args( ( MEMBER_LIST *members_list ) );
void	show_members		args( ( CHAR_DATA *ch, const std::string& argument, const std::string& format ) );


/*
 * Get pointer to clan structure from clan name.
 */
CLAN_DATA *get_clan( const std::string& name )
{
    CLAN_DATA *clan;
    
    for ( clan = first_clan; clan; clan = clan->next )
       if ( !str_cmp( name, clan->name ) )
         return clan;
    return NULL;
}

PLANET_DATA *get_planet( const std::string&name )
{
    PLANET_DATA *planet;
    
    for ( planet = first_planet; planet; planet = planet->next )
       if ( !str_cmp( name, planet->name ) )
         return planet;
    return NULL;
}

void write_clan_list( )
{
    CLAN_DATA *tclan;
    FILE *fpout;
    char filename[256];

    SPRINTF( filename, "%s%s", CLAN_DIR, CLAN_LIST );
    fpout = fopen( filename, "w" );
    if ( !fpout )
    {
	bug( "FATAL: cannot open clan.lst for writing!\n", 0 );
 	return;
    }	  
    for ( tclan = first_clan; tclan; tclan = tclan->next )
	fprintf( fpout, "%s\n", tclan->filename );
    fprintf( fpout, "$\n" );
    FCLOSE( fpout );
}

void write_planet_list( )
{
    PLANET_DATA *tplanet;
    FILE *fpout;
    char filename[256];

    SPRINTF( filename, "%s%s", PLANET_DIR, PLANET_LIST );
    fpout = fopen( filename, "w" );
    if ( !fpout )
    {
	bug( "FATAL: cannot open planet.lst for writing!\n", 0 );
 	return;
    }	  
    for ( tplanet = first_planet; tplanet; tplanet = tplanet->next )
	fprintf( fpout, "%s\n", tplanet->filename );
    fprintf( fpout, "$\n" );
    FCLOSE( fpout );
}

/*
 * Save a clan's data to its data file
 */
void save_clan( CLAN_DATA *clan )
{
    FILE *fp;
    char filename[256];

    if ( !clan )
    {
	bug( "save_clan: null clan pointer!", 0 );
	return;
    }
        
    if ( !clan->filename || clan->filename[0] == '\0' )
    {
	bug( str_printf("save_clan: %s has no filename", clan->name).c_str(), 0 );
	return;
    }
 
    SPRINTF( filename, "%s%s", CLAN_DIR, clan->filename );
    
    FCLOSE( fpReserve );
    if ( ( fp = fopen( filename, "w" ) ) == NULL )
    {
    	bug( "save_clan: fopen", 0 );
    	perror( filename );
    }
    else
    {
	fprintf( fp, "#CLAN\n" );
	fprintf( fp, "Name         %s~\n",	clan->name		);
	fprintf( fp, "Filename     %s~\n",	clan->filename		);
	fprintf( fp, "Description  %s~\n",	clan->description	);
	fprintf( fp, "Leader       %s~\n",	clan->leader		);
	fprintf( fp, "NumberOne    %s~\n",	clan->number1		);
	fprintf( fp, "NumberTwo    %s~\n",	clan->number2		);
	fprintf( fp, "PKills       %d\n",	clan->pkills		);
	fprintf( fp, "PDeaths      %d\n",	clan->pdeaths		);
	fprintf( fp, "MKills       %d\n",	clan->mkills		);
	fprintf( fp, "MDeaths      %d\n",	clan->mdeaths		);
	fprintf( fp, "Type         %d\n",	clan->clan_type		);
	fprintf( fp, "Members      %d\n",	clan->members		);
	fprintf( fp, "Board        %d\n",	clan->board		);
	fprintf( fp, "Storeroom    %d\n",	clan->storeroom		);
	fprintf( fp, "GuardOne     %d\n",	clan->guard1		);
	fprintf( fp, "GuardTwo     %d\n",	clan->guard2		);
	fprintf( fp, "PatrolOne    %d\n",	clan->patrol1		);
	fprintf( fp, "PatrolTwo    %d\n",	clan->patrol2		);
	fprintf( fp, "TrooperOne   %d\n",	clan->trooper1		);
	fprintf( fp, "TrooperTwo   %d\n",	clan->trooper2		);
	fprintf( fp, "Funds        %ld\n",	clan->funds		);
	fprintf( fp, "Enlist1      %d\n", clan->enlistroom1 );
	fprintf( fp, "Enlist2      %d\n", clan->enlistroom2 );
	fprintf( fp, "Jail         %d\n",       clan->jail            	);
	if ( clan->mainclan )
   	   fprintf( fp, "MainClan     %s~\n",	clan->mainclan->name	);
	fprintf( fp, "End\n\n"						);
	fprintf( fp, "#END\n"						);
    }
    FCLOSE( fp );
    fpReserve = fopen( NULL_FILE, "r" );
    return;
}

void save_planet( PLANET_DATA *planet )
{
    FILE *fp;
    char filename[256];

    if ( !planet )
    {
	bug( "save_planet: null planet pointer!", 0 );
	return;
    }
        
    if ( !planet->filename || planet->filename[0] == '\0' )
    {
	bug( str_printf("save_planet: %s has no filename", planet->name).c_str(), 0 );
	return;
    }
 
    SPRINTF( filename, "%s%s", PLANET_DIR, planet->filename );
    
    FCLOSE( fpReserve );
    if ( ( fp = fopen( filename, "w" ) ) == NULL )
    {
    	bug( "save_planet: fopen", 0 );
    	perror( filename );
    }
    else
    {
        AREA_DATA *pArea;
        
	fprintf( fp, "#PLANET\n" );
	fprintf( fp, "Name         %s~\n",	planet->name		);
	fprintf( fp, "Filename     %s~\n",	planet->filename        );
	fprintf( fp, "BaseValue    %ld\n",	planet->base_value      );
	fprintf( fp, "Flags        %d\n",	planet->flags           );
	fprintf( fp, "PopSupport   %f\n",	planet->pop_support      );
	if ( planet->spaceobject && planet->spaceobject->name )
        	fprintf( fp, "spaceobject   %s~\n",	planet->spaceobject->name);
	if ( planet->governed_by && planet->governed_by->name )
        	fprintf( fp, "GovernedBy   %s~\n",	planet->governed_by->name);
	for( pArea = planet->first_area ; pArea ; pArea = pArea->next_on_planet )
	    if (pArea->filename)
         	fprintf( fp, "Area         %s~\n",	pArea->filename  );
	fprintf( fp, "End\n\n"						);
	fprintf( fp, "#END\n"						);
    }
    FCLOSE( fp );
    fpReserve = fopen( NULL_FILE, "r" );
    return;
}


/*
 * Read in actual clan data.
 */

#if defined(KEY)
#undef KEY
#endif

#define KEY( literal, field, value )					\
				if ( !str_cmp( word, literal ) )	\
				{					\
				    field  = value;			\
				    fMatch = TRUE;			\
				    break;				\
				}

void fread_clan( CLAN_DATA *clan, FILE *fp )
{
    const char *word;
    bool fMatch;

    for ( ; ; )
    {
	word   = feof( fp ) ? "End" : fread_word( fp );
	fMatch = FALSE;

	switch ( UPPER(word[0]) )
	{
	case '*':
	    fMatch = TRUE;
	    fread_to_eol( fp );
	    break;

	case 'B':
	    KEY( "Board",	clan->board,		fread_number( fp ) );
	    break;

	case 'D':
	    KEY( "Description",	clan->description,	fread_string( fp ) );
	    break;

	case 'E':
            KEY( "Enlist1",	clan->enlistroom1,		fread_number( fp ) );
            KEY( "Enlist2",	clan->enlistroom2,		fread_number( fp ) );
	    if ( !str_cmp( word, "End" ) )
	    {
		if (!clan->name)
		  clan->name		= STRALLOC( "" );
		if (!clan->leader)
		  clan->leader		= STRALLOC( "" );
		if (!clan->description)
		  clan->description 	= STRALLOC( "" );
		if (!clan->number1)
		  clan->number1		= STRALLOC( "" );
		if (!clan->number2)
		  clan->number2		= STRALLOC( "" );
		if (!clan->tmpstr)
		  clan->tmpstr		= STRALLOC( "" );
		return;
	    }
	    break;
	    
	case 'F':
	    KEY( "Funds",	clan->funds,		fread_number( fp ) );
	    KEY( "Filename",	clan->filename,		fread_string_nohash( fp ) );
	    break;

	case 'G':
	    KEY( "GuardOne",	clan->guard1,		fread_number( fp ) );
	    KEY( "GuardTwo",	clan->guard2,		fread_number( fp ) );
	    break;

	case 'J':
	    KEY( "Jail",	clan->jail,		fread_number( fp ) );
	    break;

	case 'L':
	    KEY( "Leader",	clan->leader,		fread_string( fp ) );
	    break;

	case 'M':
	    KEY( "MDeaths",	clan->mdeaths,		fread_number( fp ) );
	    KEY( "Members",	clan->members,		fread_number( fp ) );
	    KEY( "MKills",	clan->mkills,		fread_number( fp ) );
	    KEY( "MainClan",	clan->tmpstr,		fread_string( fp ) );
	    break;
 
	case 'N':
	    KEY( "Name",	clan->name,		fread_string( fp ) );
	    KEY( "NumberOne",	clan->number1,		fread_string( fp ) );
	    KEY( "NumberTwo",	clan->number2,		fread_string( fp ) );
	    break;

	case 'P':
	    KEY( "PDeaths",	clan->pdeaths,		fread_number( fp ) );
	    KEY( "PKills",	clan->pkills,		fread_number( fp ) );
	    KEY( "PatrolOne",	clan->patrol1,		fread_number( fp ) );
	    KEY( "PatrolTwo",	clan->patrol2,		fread_number( fp ) );
	    break;

	case 'S':
	    KEY( "Storeroom",	clan->storeroom,	fread_number( fp ) );
	    break;

	case 'T':
	    KEY( "Type",	clan->clan_type,	fread_number( fp ) );
	    KEY( "TrooperOne",	clan->trooper1,		fread_number( fp ) );
	    KEY( "TrooperTwo",	clan->trooper2,		fread_number( fp ) );
	    break;
	}
	
	if ( !fMatch )
	{
	    bug( str_printf("Fread_clan: no match: %s", word).c_str(), 0 );
	}
	
    }
}

void fread_planet( PLANET_DATA *planet, FILE *fp )
{
    const char *word;
    bool fMatch;

    for ( ; ; )
    {
	word   = feof( fp ) ? "End" : fread_word( fp );
	fMatch = FALSE;

	switch ( UPPER(word[0]) )
	{
	case '*':
	    fMatch = TRUE;
	    fread_to_eol( fp );
	    break;

	case 'A':
	    if ( !str_cmp( word, "Area" ) )
	    {
	        char aName[MAX_STRING_LENGTH];
                AREA_DATA *pArea;
                	        
	     	SPRINTF (aName, "%s", fread_string(fp));
		for( pArea = first_area ; pArea ; pArea = pArea->next )
	          if (pArea->filename && !str_cmp(pArea->filename , aName ) )
	          {
	             pArea->planet = planet; 
	             LINK( pArea, planet->first_area, planet->last_area, next_on_planet, prev_on_planet);
	          }      
                fMatch = TRUE;
	    }
	    break;

	case 'B':
	    KEY( "BaseValue",	planet->base_value,		fread_number( fp ) );
	    break;

	case 'E':
	    if ( !str_cmp( word, "End" ) )
	    {
		if (!planet->name)
		  planet->name		= STRALLOC( "" );
		return;
	    }
	    break;

	case 'F':
	    KEY( "Filename",	planet->filename,		fread_string_nohash( fp ) );
	    KEY( "Flags",	planet->flags,		        fread_number( fp ) );
	    break;
	
	case 'G':
	    if ( !str_cmp( word, "GovernedBy" ) )
	    {
            char *clan_name = fread_string( fp );
            planet->governed_by = get_clan( std::string(clan_name) );
            fMatch = TRUE;
            STRFREE( clan_name );
	    }
	    break;
	
	case 'N':
	    KEY( "Name",	planet->name,		fread_string( fp ) );
	    break;
	
	case 'P':
	    KEY( "PopSupport",	planet->pop_support,		fread_float( fp ) );
	    break;

	case 'S':
	    if ( !str_cmp( word, "spaceobject" ) )
	    {
            char *spaceobject_name = fread_string( fp );
	     	planet->spaceobject = spaceobject_from_name ( planet->game, std::string(spaceobject_name) );
            STRFREE( spaceobject_name );
            if (planet->spaceobject)
            {
                    SPACE_DATA *spaceobject = planet->spaceobject;
                    
                spaceobject->planet = planet;
            }
            fMatch = TRUE;
	    }
	    break;
	
	case 'T':
	    KEY( "Taxes",	planet->base_value,		fread_number( fp ) );
	    break;
    
	}
	
	if ( !fMatch )
	{
	    bug( str_printf("Fread_planet: no match: %s", word).c_str(), 0 );
	}
	
    }
}


/*
 * Load a clan file
 */

bool load_clan_file( GameContext *game, const std::string& clanfile )
{
    char filename[256];
    CLAN_DATA *clan;
    FILE *fp;
    bool found;

    CREATE( clan, CLAN_DATA, 1 );
    clan->game = game;
    clan->next_subclan = NULL;
    clan->prev_subclan = NULL;
    clan->last_subclan = NULL;
    clan->first_subclan = NULL;    
    clan->mainclan     = NULL;
    
    found = FALSE;
    SPRINTF( filename, "%s%s", CLAN_DIR, clanfile.c_str() );

    if ( ( fp = fopen( filename, "r" ) ) != NULL )
    {

	found = TRUE;
	for ( ; ; )
	{
	    char letter;
	    char *word;

	    letter = fread_letter( fp );
	    if ( letter == '*' )
	    {
		fread_to_eol( fp );
		continue;
	    }

	    if ( letter != '#' )
	    {
		bug( "Load_clan_file: # not found.", 0 );
		break;
	    }

	    word = fread_word( fp );
	    if ( !str_cmp( word, "CLAN"	) )
	    {
	    	fread_clan( clan, fp );
	    	break;
	    }
	    else
	    if ( !str_cmp( word, "END"	) )
	        break;
	    else
	    {
		bug( str_printf("Load_clan_file: bad section: %s.", word).c_str(), 0 );
		break;
	    }
	}
	FCLOSE( fp );
    }

    if ( found )
    {
	ROOM_INDEX_DATA *storeroom;

	LINK( clan, first_clan, last_clan, next, prev );

         if( !load_member_list( game, clan->filename ) )
         {
             MEMBER_LIST *members_list;

             log_string( "No memberlist found, creating new list" );
             CREATE( members_list, MEMBER_LIST, 1 );
             members_list->game = game;
             members_list->name = STRALLOC( clan->name );
             LINK( members_list, first_member_list, last_member_list, next, prev );
             save_member_list( members_list );
         }

	if ( clan->storeroom == 0
	|| (storeroom = get_room_index( clan->storeroom )) == NULL )
	{
	    log_string( "Storeroom not found" );
	    return found;
	}
	
	SPRINTF( filename, "%s%s.vault", CLAN_DIR, clan->filename );
	if ( ( fp = fopen( filename, "r" ) ) != NULL )
	{
//	    bool found;
	    OBJ_DATA *tobj, *tobj_next;

	    log_string( "Loading clan storage room" );
	    rset_supermob(storeroom);

//	    found = TRUE;
	    for ( ; ; )
	    {
		char letter;
		char *word;

		letter = fread_letter( fp );
		if ( letter == '*' )
		{
		    fread_to_eol( fp );
		    continue;
		}

		if ( letter != '#' )
		{
		    bug( "Load_clan_vault: # not found.", 0 );
		    bug( clan->name, 0 );
		    break;
		}

		word = fread_word( fp );
		if ( !str_cmp( word, "OBJECT" ) )	/* Objects	*/
		  fread_obj  ( game, supermob, fp, OS_CARRY );
		else
		if ( !str_cmp( word, "END"    ) )	/* Done		*/
		  break;
		else
		{
		    bug( "Load_clan_vault: bad section.", 0 );
		    bug( clan->name, 0 );
		    break;
		}
	    }
	    FCLOSE( fp );
	    for ( tobj = supermob->first_carrying; tobj; tobj = tobj_next )
	    {
		tobj_next = tobj->next_content;
		obj_from_char( tobj );
		obj_to_room( tobj, storeroom );
	    }
	    release_supermob(game);
	}
	else
	    log_string( "Cannot open clan vault" );
    }
    else
      DISPOSE( clan );

    return found;
}

bool load_planet_file( GameContext *game, const std::string& planetfile )
{
    char filename[256];
    PLANET_DATA *planet;
    FILE *fp;
    bool found;

    CREATE( planet, PLANET_DATA, 1 );
    planet->game = game;
    
    planet->governed_by = NULL;
    planet->next_in_system = NULL;
    planet->prev_in_system = NULL;
    planet->spaceobject = NULL ;
    planet->first_area = NULL;
    planet->last_area = NULL;
    planet->first_guard = NULL;
    planet->last_guard = NULL;
    
    found = FALSE;
    SPRINTF( filename, "%s%s", PLANET_DIR, planetfile.c_str() );

    if ( ( fp = fopen( filename, "r" ) ) != NULL )
    {

	found = TRUE;
	for ( ; ; )
	{
	    char letter;
	    char *word;

	    letter = fread_letter( fp );
	    if ( letter == '*' )
	    {
		fread_to_eol( fp );
		continue;
	    }

	    if ( letter != '#' )
	    {
		bug( "Load_planet_file: # not found.", 0 );
		break;
	    }

	    word = fread_word( fp );
	    if ( !str_cmp( word, "PLANET"	) )
	    {
	    	fread_planet( planet, fp );
	    	break;
	    }
	    else
	    if ( !str_cmp( word, "END"	) )
	        break;
	    else
	    {
		bug( str_printf("Load_planet_file: bad section: %s.", word).c_str(), 0 );
		break;
	    }
	}
	FCLOSE( fp );
    }

    if ( !found )
      DISPOSE( planet );
    else
      LINK( planet, first_planet, last_planet, next, prev );

    return found;
}


/*
 * Load in all the clan files.
 */
void load_clans( GameContext *game )
{
    FILE *fpList;
    const char *filename;
    char clanlist[256];
    CLAN_DATA *clan;
    CLAN_DATA *bosclan;
    
    first_clan	= NULL;
    last_clan	= NULL;

    log_string( "Loading clans..." );

    SPRINTF( clanlist, "%s%s", CLAN_DIR, CLAN_LIST );
    FCLOSE( fpReserve );
    if ( ( fpList = fopen( clanlist, "r" ) ) == NULL )
    {
	perror( clanlist );
	exit( 1 );
    }

    for ( ; ; )
    {
	filename = feof( fpList ) ? "$" : fread_word( fpList );
	log_string( filename );
	if ( filename[0] == '$' )
	  break;

	if ( !load_clan_file( game, std::string( filename ) ) )
	{
	  bug( str_printf("Cannot load clan file: %s", filename).c_str(), 0 );
	}
    }
    FCLOSE( fpList );
    log_string(" Done clans\nSorting clans...." );
    fpReserve = fopen( NULL_FILE, "r" );
    
    for ( clan=first_clan ; clan ; clan = clan->next )
    {
       if ( !clan->tmpstr || clan->tmpstr[0] == '\0' )
         continue;
         
       bosclan = get_clan ( std::string(clan->tmpstr) );
       if ( !bosclan ) 
         continue;
         
       LINK( clan , bosclan->first_subclan , bosclan->last_subclan , next_subclan, prev_subclan );
       clan->mainclan = bosclan;
    }
    
    log_string(" Done sorting" );
    return;
}

void load_planets( GameContext *game )
{
    FILE *fpList;
    const char *filename;
    char planetlist[256];
    
    first_planet	= NULL;
    last_planet	= NULL;

    log_string( "Loading planets..." );

    SPRINTF( planetlist, "%s%s", PLANET_DIR, PLANET_LIST );
    FCLOSE( fpReserve );
    if ( ( fpList = fopen( planetlist, "r" ) ) == NULL )
    {
	perror( planetlist );
	exit( 1 );
    }

    for ( ; ; )
    {
	filename = feof( fpList ) ? "$" : fread_word( fpList );
	log_string( filename );
	if ( filename[0] == '$' )
	  break;

	if ( !load_planet_file( game, (char * ) filename ) )
	{
	  bug( str_printf("Cannot load planet file: %s", filename).c_str(), 0 );
	}
    }
    FCLOSE( fpList );
    log_string(" Done planets " );  
    fpReserve = fopen( NULL_FILE, "r" );
    return;
}

void do_make( CHAR_DATA *ch, char *argument )
{
	send_to_char( "Huh?\n", ch );
	return;
}

void do_induct( CHAR_DATA *ch, char *argument )
{
    std::string arg;
    CHAR_DATA *victim;
    CLAN_DATA *clan;

    if ( IS_NPC( ch ) || !ch->pcdata->clan )
    {
	send_to_char( "Huh?\n", ch );
	return;
    }

    clan = ch->pcdata->clan;
    
    if ( (ch->pcdata && ch->pcdata->bestowments
    &&    is_name("induct", ch->pcdata->bestowments))
    ||   !str_cmp( ch->name, clan->leader  )
    ||   !str_cmp( ch->name, clan->number1 )
    ||   !str_cmp( ch->name, clan->number2 ) )
	;
    else
    {
	send_to_char( "Huh?\n", ch );
	return;
    }

    one_argument( argument, arg );

    if ( arg.empty() )
    {
	send_to_char( "Induct whom?\n", ch );
	return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
	send_to_char( "That player is not here.\n", ch);
	return;
    }

    if ( IS_NPC(victim) )
    {
	send_to_char( "Not on NPC's.\n", ch );
	return;
    }

    if ( victim->pcdata->clan )
    {
      if ( victim->pcdata->clan->clan_type == CLAN_CRIME )
      {
	if ( victim->pcdata->clan == clan )
	  send_to_char( "This player already belongs to your crime family!\n", ch );
	else
	  send_to_char( "This player already belongs to an organization!\n", ch );
	return;
      }
      else if ( victim->pcdata->clan->clan_type == CLAN_GUILD )
      {
	if ( victim->pcdata->clan == clan )
	  send_to_char( "This player already belongs to your guild!\n", ch );
	else
	  send_to_char( "This player already belongs to an organization!\n", ch );
	return;
      }
      else
      {
	if ( victim->pcdata->clan == clan )
	  send_to_char( "This player already belongs to your organization!\n", ch );
	else
	  send_to_char( "This player already belongs to an organization!\n", ch );
	return;
      }
      
    }
    
    clan->members++;

    victim->pcdata->clan = clan;
    STRFREE(victim->pcdata->clan_name);
    victim->pcdata->clan_name = QUICKLINK( clan->name );
    update_member( victim );
    act( AT_MAGIC, "You induct $N into $t", ch, clan->name, victim, TO_CHAR );
    act( AT_MAGIC, "$n inducts $N into $t", ch, clan->name, victim, TO_NOTVICT );
    act( AT_MAGIC, "$n inducts you into $t", ch, clan->name, victim, TO_VICT );
    save_char_obj( victim );
    return;
}

/* Can the character outcast the victim? - FUSS/DV 3-13-26 */
bool can_outcast( CLAN_DATA *clan, CHAR_DATA *ch, CHAR_DATA *victim )
{
   if( !clan || !ch || !victim )
      return FALSE;
   if( !str_cmp( ch->name, clan->leader ) )
      return TRUE;
   if( !str_cmp( victim->name, clan->leader ) )
      return FALSE;
   if( !str_cmp( ch->name, clan->number1 ) )
      return TRUE;
   if( !str_cmp( victim->name, clan->number1 ) )
      return FALSE;
   if( !str_cmp( ch->name, clan->number2 ) )
      return TRUE;
   if( !str_cmp( victim->name, clan->number2 ) )
      return FALSE;
   return TRUE;
}

void do_outcast( CHAR_DATA *ch, char *argument )
{
    std::string arg;
    CHAR_DATA *victim;
    CLAN_DATA *clan;

    if ( IS_NPC( ch ) || !ch->pcdata->clan )
    {
	send_to_char( "Huh?\n", ch );
	return;
    }

    clan = ch->pcdata->clan;

    if ( (ch->pcdata && ch->pcdata->bestowments
    &&    is_name("outcast", ch->pcdata->bestowments))
    ||   !str_cmp( ch->name, clan->leader  )
    ||   !str_cmp( ch->name, clan->number1 )
    ||   !str_cmp( ch->name, clan->number2 ) )
	;
    else
    {
	send_to_char( "Huh?\n", ch );
	return;
    }


    one_argument( argument, arg );

    if ( arg.empty() )
    {
	send_to_char( "Outcast whom?\n", ch );
	return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
	send_to_char( "That player is not here.\n", ch);
	return;
    }

    if ( IS_NPC(victim) )
    {
	send_to_char( "Not on NPC's.\n", ch );
	return;
    }

    if ( victim == ch )
    {
	    send_to_char( "Kick yourself out of your own clan?\n", ch );
	    return;
    }
 
    if ( victim->pcdata->clan != ch->pcdata->clan )
    {
	    send_to_char( "This player does not belong to your clan!\n", ch );
	    return;
    }
    if( !can_outcast( clan, ch, victim ) )
    {
        send_to_char( "You are not able to outcast them.\n", ch );
        return;
    }

    if ( victim->speaking & LANG_CLAN )
        victim->speaking = LANG_COMMON;
    BV_REMOVE_BIT( victim->speaks, LANG_CLAN );
    --clan->members;
    if( clan->members < 0 ) //--Shaddai/FUSS - DV added 3-13-26
        clan->members = 0;    
    if ( !str_cmp( victim->name, ch->pcdata->clan->number1 ) )
    {
	STRFREE( ch->pcdata->clan->number1 );
	ch->pcdata->clan->number1 = STRALLOC( "" );
    }
    if ( !str_cmp( victim->name, ch->pcdata->clan->number2 ) )
    {
	STRFREE( ch->pcdata->clan->number2 );
	ch->pcdata->clan->number2 = STRALLOC( "" );
    }
    victim->pcdata->clan = NULL;
    remove_member( victim );
    STRFREE(victim->pcdata->clan_name);
    victim->pcdata->clan_name = STRALLOC( "" );
    act( AT_MAGIC, "You outcast $N from $t", ch, clan->name, victim, TO_CHAR );
    act( AT_MAGIC, "$n outcasts $N from $t", ch, clan->name, victim, TO_ROOM );
    act( AT_MAGIC, "$n outcasts you from $t", ch, clan->name, victim, TO_VICT );
    
    STR_DISPOSE( victim->pcdata->bestowments );
    victim->pcdata->bestowments = str_dup("");
    
    save_char_obj( victim );	/* clan gets saved when pfile is saved */
    return;
}

void do_setclan( CHAR_DATA *ch, char *argument )
{
    std::string arg1;
    std::string arg2;
    std::string argstr = argument;
    CLAN_DATA *clan;

    if ( IS_NPC( ch ) )
    {
	send_to_char( "Huh?\n", ch );
	return;
    }

    argstr = one_argument( argstr, arg1 );
    argstr = one_argument( argstr, arg2 );

    if ( arg1.empty() )
    {
	send_to_char( "Usage: setclan <clan> <field> <leader|number1|number2> <player>\n", ch );
	send_to_char( "\nField being one of:\n", ch );
	send_to_char( " leader number1 number2 subclan enlist1\n", ch ); 
	send_to_char( " enlist2 members board recall storage\n", ch );
	send_to_char( " funds trooper1 trooper2 jail", ch );	
	send_to_char( " guard1 guard2 patrol1 patrol2\n", ch );
	if ( get_trust( ch ) >= LEVEL_SUB_IMPLEM )
	{
	  send_to_char( " name filename desc\n", ch );
	}
	return;
    }

    clan = get_clan( arg1 );
    if ( !clan )
    {
	send_to_char( "No such clan.\n", ch );
	return;
    }

	if ( !str_cmp( arg2, "enlistroom1" ) )
	{
		clan->enlistroom1 = strtoi( argstr );
		send_to_char( "Done.\n", ch );
		save_clan( clan );
		return;
	}
	if ( !str_cmp( arg2, "enlistroom2" ) )
	{
		clan->enlistroom2 = strtoi( argstr );
		send_to_char( "Done.\n", ch );
		save_clan( clan );
		return;
	}
    if ( !str_cmp( arg2, "leader" ) )
    {
	STRFREE( clan->leader );
	clan->leader = STRALLOC( argstr );
	send_to_char( "Done.\n", ch );
	save_clan( clan );
	return;
    }

    if ( !str_cmp( arg2, "subclan" ) )
    {
        CLAN_DATA *subclan;
        subclan = get_clan( argstr );
        if ( !subclan )
        {
            send_to_char( "Subclan is not a clan.\n", ch );
            return;
        }
        if ( subclan->clan_type == CLAN_SUBCLAN || subclan->mainclan )
        {
            send_to_char( "Subclan is already part of another organization.\n", ch );
            return;
        }
        if ( subclan->first_subclan )
        {
            send_to_char( "Subclan has subclans of its own that need removing first.\n", ch );
            return;
        }
        subclan->clan_type = CLAN_SUBCLAN;
        subclan->mainclan = clan;
        LINK(subclan, clan->first_subclan, clan->last_subclan, next_subclan, prev_subclan );
	save_clan( clan );
	save_clan( subclan );
	return;
    }

    if ( !str_cmp( arg2, "number1" ) )
    {
	STRFREE( clan->number1 );
	clan->number1 = STRALLOC( argstr );
	send_to_char( "Done.\n", ch );
	save_clan( clan );
	return;
    }

    if ( !str_cmp( arg2, "number2" ) )
    {
	STRFREE( clan->number2 );
	clan->number2 = STRALLOC( argstr );
	send_to_char( "Done.\n", ch );
	save_clan( clan );
	return;
    }

    if ( !str_cmp( arg2, "board" ) )
    {
	clan->board = strtoi( argstr );
	send_to_char( "Done.\n", ch );
	save_clan( clan );
	return;
    }

    if ( !str_cmp( arg2, "members" ) )
    {
	clan->members = strtoi( argstr );
	send_to_char( "Done.\n", ch );
	save_clan( clan );
	return;
    }

    if ( !str_cmp( arg2, "funds" ) )
    {
	clan->funds = strtoi( argstr );
	send_to_char( "Done.\n", ch );
	save_clan( clan );
	return;
    }

    if ( !str_cmp( arg2, "storage" ) )
    {
	clan->storeroom = strtoi( argstr );
	send_to_char( "Done.\n", ch );
	save_clan( clan );
	return;
    }

    if ( !str_cmp( arg2, "guard1" ) )
    {
	clan->guard1 = strtoi( argstr );
	send_to_char( "Done.\n", ch );
	save_clan( clan );
	return;
    }

    if ( !str_cmp( arg2, "jail" ) )
    {
	clan->jail = strtoi( argstr );
	send_to_char( "Done.\n", ch );
	save_clan( clan );
	return;
    }

    if ( !str_cmp( arg2, "guard2" ) )
    {
	clan->guard2 = strtoi( argstr );
	send_to_char( "Done.\n", ch );
	save_clan( clan );
	return;
    }

    if ( !str_cmp( arg2, "trooper1" ) )
    {
	clan->trooper1 = strtoi( argstr );
	send_to_char( "Done.\n", ch );
	save_clan( clan );
	return;
    }

    if ( !str_cmp( arg2, "trooper2" ) )
    {
	clan->trooper2 = strtoi( argstr );
	send_to_char( "Done.\n", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "patrol1" ) )
    {
	clan->patrol1 = strtoi( argstr );
	send_to_char( "Done.\n", ch );
	save_clan( clan );
	return;
    }

    if ( !str_cmp( arg2, "patrol2" ) )
    {
	clan->patrol2 = strtoi( argstr );
	send_to_char( "Done.\n", ch );
	save_clan( clan );
	return;
    }

    if ( get_trust( ch ) < LEVEL_SUB_IMPLEM )
    {
	do_setclan( ch, "" );
	return;
    }
    
    if ( !str_cmp( arg2, "type" ) )
    {
        if ( clan->mainclan )
        {
            UNLINK ( clan , clan->mainclan->first_subclan, clan->mainclan->last_subclan, next_subclan, prev_subclan );
            clan->mainclan = NULL;
	}
	if ( !str_cmp( argstr, "crime" ) )
	  clan->clan_type = CLAN_CRIME;
	else
	if ( !str_cmp( argstr, "crime family" ) )
	  clan->clan_type = CLAN_CRIME;
	else
	if ( !str_cmp( argstr, "guild" ) )
	  clan->clan_type = CLAN_GUILD;
	else
	  clan->clan_type = 0;
	send_to_char( "Done.\n", ch );
	save_clan( clan );
	return;
    }

   if( !str_cmp( arg2, "name" ) )
   {
      CLAN_DATA *uclan = NULL;

      if( argstr.empty() )
      {
         send_to_char( "You can't name a clan nothing.\n", ch );
         return;
      }
      if( ( uclan = get_clan( argstr ) ) )
      {
         send_to_char( "There is already another clan with that name.\n", ch );
         return;
      }
      STRFREE( clan->name );
      clan->name = STRALLOC( argstr );
      send_to_char( "Done.\n", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "filename" ) )
   {
      char filename[256];

      if( !is_valid_filename( ch, CLAN_DIR, argstr ) )
         return;

      snprintf( filename, sizeof( filename ), "%s%s", CLAN_DIR, clan->filename );
      if( !remove( filename ) )
         send_to_char( "Old clan file deleted.\n", ch );

      STR_DISPOSE( clan->filename );
      clan->filename = str_dup( argstr );
      send_to_char( "Done.\n", ch );
      save_clan( clan );
      write_clan_list(  );
      return;
   }

    if ( !str_cmp( arg2, "desc" ) )
    {
	STRFREE( clan->description );
	clan->description = STRALLOC( argstr );
	send_to_char( "Done.\n", ch );
	save_clan( clan );
	return;
    }

    do_setclan( ch, "" );
    return;
}

void do_setplanet( CHAR_DATA *ch, char *argument )
{
    std::string arg1;
    std::string arg2;
    std::string argstr = argument;
    PLANET_DATA *planet;

    if ( IS_NPC( ch ) )
    {
	send_to_char( "Huh?\n", ch );
	return;
    }

    argstr = one_argument( argstr, arg1 );
    argstr = one_argument( argstr, arg2 );

    if ( arg1.empty() )
    {
	send_to_char( "Usage: setplanet <planet> <field> [value]\n", ch );
	send_to_char( "\nField being one of:\n", ch );
	send_to_char( " base_value flags\n", ch ); 
	send_to_char( " name filename spaceobject governed_by\n", ch );
	return;
    }

    planet = get_planet( arg1 );
    if ( !planet )
    {
	send_to_char( "No such planet.\n", ch );
	return;
    }


    if( !str_cmp( arg2, "name" ) )
    {
        PLANET_DATA *tplanet;
        if( argstr.empty() )
        {
            send_to_char( "You must choose a name.\n", ch );
            return;
        }
        if( ( tplanet = get_planet( argstr ) ) != NULL )
        {
            send_to_char( "A planet with that name already Exists!\n", ch );
            return;
        }

        STRFREE( planet->name );
        planet->name = STRALLOC( argstr );
        send_to_char( "Done.\n", ch );
        save_planet( planet );
        return;
    }

    if ( !str_cmp( arg2, "governed_by" ) )
    {
        CLAN_DATA *clan;
        clan = get_clan( argstr );
        if ( clan )
        { 
           planet->governed_by = clan;
           send_to_char( "Done.\n", ch ); 
       	   save_planet( planet );
        }
        else
           send_to_char( "No such clan.\n", ch ); 
	return;
    }

    if ( !str_cmp( arg2, "spaceobject" ) )
    {
        SPACE_DATA *spaceobject;
        
	if ( (planet->spaceobject = spaceobject_from_name(planet->game, argstr)) )
	{
         if ((spaceobject=planet->spaceobject) != NULL)
         {
           spaceobject = planet->spaceobject;
           spaceobject->planet = planet;

           send_to_char( "Done.\n", ch );
	 }
	else 
	       	send_to_char( "No such spaceobject.\n", ch );
    }
	save_planet( planet );
	return;
    }

    if( !str_cmp( arg2, "filename" ) )
    {
        PLANET_DATA *tplanet;

        if( argstr.empty() )
        {
            send_to_char( "You must choose a file name.\n", ch );
            return;
        }
        for( tplanet = first_planet; tplanet; tplanet = tplanet->next )
        {
            if( !str_cmp( tplanet->filename, argstr ) )
            {
                send_to_char( "A planet with that filename already exists!\n", ch );
                return;
            }
        }

        STR_DISPOSE( planet->filename );
        planet->filename = str_dup( argstr );
        send_to_char( "Done.\n", ch );
        save_planet( planet );
        write_planet_list(  );
        return;
    }

    if ( !str_cmp( arg2, "base_value" ) )
    {
	planet->base_value = strtoi( argstr );
	send_to_char( "Done.\n", ch );
	save_planet( planet );
	return;
    }

    if ( !str_cmp( arg2, "flags" ) )
    {
        std::string farg;
        
        argstr = one_argument( argstr, farg); 
        
        if ( farg.empty() )
        {
           send_to_char( "Possible flags: nocapture\n", ch );
           return;
        }
        
        for ( ; !farg.empty() ; argstr = one_argument( argstr, farg) )
        {
            if ( !str_cmp( farg, "nocapture" ) )
               TOGGLE_BIT( planet->flags, PLANET_NOCAPTURE );
            else
               ch_printf( ch , "No such flag: %s\n" , farg );
	}
	send_to_char( "Done.\n", ch );
	save_planet( planet );
	return;
    }

    do_setplanet( ch, "" );
    return;
}

void do_showclan( CHAR_DATA *ch, char *argument )
{   
    CLAN_DATA *clan;

    if ( IS_NPC( ch ) )
    {
	send_to_char( "Huh?\n", ch );
	return;
    }

    if ( argument[0] == '\0' )
    {
	send_to_char( "Usage: showclan <clan>\n", ch );
	return;
    }

    clan = get_clan( argument );
    if ( !clan )
    {
	send_to_char( "No such clan.\n", ch );
	return;
    }

    ch_printf( ch, "%s      : %s\nFilename: %s\n",
			clan->clan_type == CLAN_CRIME ? "Crime Family " : 
			    clan->clan_type == CLAN_GUILD ? "Guild " : "Organization ",
    			clan->name,
    			clan->filename);
    ch_printf( ch, "Description: %s\nLeader: %s\n",
    			clan->description,
    			clan->leader );
    ch_printf( ch, "Number1: %s\nNumber2: %s\nPKills: %6d    PDeaths: %6d\n",
    			clan->number1,
    			clan->number2,
    			clan->pkills,
    			clan->pdeaths );
    ch_printf( ch, "MKills: %6d    MDeaths: %6d\n",
    			clan->mkills,
    			clan->mdeaths );
    ch_printf( ch, "Type: %d\n",
    			clan->clan_type );
    ch_printf( ch, "Members: %3d\n",
    			clan->members );
    ch_printf( ch, "Board: %5d   Jail: %5d\n",
    			clan->board, clan->jail);
    ch_printf( ch, "Guard1: %5d  Guard2: %5d\n",
    			clan->guard1,
    			clan->guard2 );
    ch_printf( ch, "Patrol1: %5d  Patrol2: %5d\n",
    			clan->patrol1,
    			clan->patrol2 );
    ch_printf( ch, "Trooper1: %5d  Trooper2: %5d\n",
    			clan->trooper1,
    			clan->trooper2 );
    ch_printf( ch, "Funds: %ld\n",
    			clan->funds );
    ch_printf( ch, "Enlist Room 1: %ld  Enlist Room 2: %ld\n",
    			clan->enlistroom1, clan->enlistroom2 );
    return;
}

void do_showplanet( CHAR_DATA *ch, char *argument )
{   
    PLANET_DATA *planet;

    if ( IS_NPC( ch ) )
    {
	send_to_char( "Huh?\n", ch );
	return;
    }

    if ( argument[0] == '\0' )
    {
	send_to_char( "Usage: showplanet <planet>\n", ch );
	return;
    }

    planet = get_planet( argument );
    if ( !planet )
    {
	send_to_char( "No such planet.\n", ch );
	return;
    }

    ch_printf( ch, "%s\nFilename: %s\nStarsystem: %s\n",
    			planet->name,
    			planet->filename,
    			planet->spaceobject ? planet->spaceobject->name : "None");
    return;
}

void do_makeclan( CHAR_DATA *ch, char *argument )
{
    char filename[256];
    CLAN_DATA *clan;
//    bool found;

    if ( !argument || argument[0] == '\0' )
    {
	send_to_char( "Usage: makeclan <clan name>\n", ch );
	return;
    }

//    found = FALSE;
    SPRINTF( filename, "%s%s", CLAN_DIR, strlower(argument).c_str() );

    CREATE( clan, CLAN_DATA, 1 );
    clan->game = ch->game;
    LINK( clan, first_clan, last_clan, next, prev );
    clan->next_subclan = NULL;
    clan->prev_subclan = NULL;
    clan->last_subclan = NULL;
    clan->first_subclan = NULL;    
    clan->mainclan     = NULL;
    clan->name		= STRALLOC( argument );
    clan->description	= STRALLOC( "" );
    clan->leader	= STRALLOC( "" );
    clan->number1	= STRALLOC( "" );
    clan->number2	= STRALLOC( "" );
    clan->tmpstr	= STRALLOC( "" );
}

void do_makeplanet( CHAR_DATA *ch, char *argument )
{
    char filename[256];
    PLANET_DATA *planet;
//    bool found;

    if ( !argument || argument[0] == '\0' )
    {
	send_to_char( "Usage: makeplanet <planet name>\n", ch );
	return;
    }

//    found = FALSE;
    SPRINTF( filename, "%s%s", PLANET_DIR, strlower(argument).c_str() );

    CREATE( planet, PLANET_DATA, 1 );
    planet->game = ch->game;
    LINK( planet, first_planet, last_planet, next, prev );
    planet->governed_by = NULL;
    planet->next_in_system = NULL;
    planet->prev_in_system = NULL;
    planet->spaceobject = NULL ;
    planet->first_area = NULL;
    planet->last_area = NULL;
    planet->first_guard = NULL;
    planet->last_guard = NULL;
    planet->name		= STRALLOC( argument );
    planet->flags               = 0;
}

void do_clans( CHAR_DATA *ch, char *argument )
{
    CLAN_DATA *clan;
    PLANET_DATA *planet;
    int count = 0;
    int pCount = 0;
    float support;
    float revenue;
        
    for ( clan = first_clan; clan; clan = clan->next )
    {
        if ( clan->clan_type == CLAN_CRIME || clan->clan_type == CLAN_GUILD || clan->clan_type == CLAN_SUBCLAN )
          continue;
        
        pCount = 0;
        support = 0;
        revenue = 0;
        
        for ( planet = first_planet ; planet ; planet = planet->next )
          if ( clan == planet->governed_by )
          {
            support += planet->pop_support;
            pCount++;
            revenue += get_taxes(planet);
          }
          
        if ( pCount > 1 )
           support /= pCount;
	ch_printf( ch, "--------------------------------------------------------------------------------\n");
        ch_printf( ch, "&z&WOrganization: &Y%-20s    ", clan->name);
	ch_printf( ch, "&WPlanets: &O%-4d &z&WAvg Pop Support: ",pCount);
	if (support <50)
	{
		ch_printf( ch, "&R");
	}
	else
	{
		ch_printf( ch, "&O");
	}
	ch_printf( ch,"%-3.1f&W\nRevenue: &O%-29.0f",support,revenue);
	ch_printf(ch,"&z&WLeader : ");
	if( clan->leader[0] != 0 )
	{
		ch_printf(ch,"&O%-20s",clan->leader);
	}
	else
	{
		ch_printf(ch,"&RNONE.             ");
	}
	ch_printf(ch, "&W\n");
        if ( clan->first_subclan )
        {
           CLAN_DATA *subclan;
           ch_printf( ch, "  &z&wSubclans             Leader\n");
           
           for ( subclan = clan->first_subclan ; subclan ; subclan = subclan->next_subclan ) 
           {
               ch_printf( ch, "  &O%-20s %-10s\n",
                  subclan->name, subclan->leader );
           }
        }
        count++;
    }
    ch_printf( ch, "--------------------------------------------------------------------------------\n");
    ch_printf( ch, "&z&WAutonomous Groups        Leader\n");
    for ( clan = first_clan; clan; clan = clan->next )
    {
        if ( clan->clan_type != CLAN_CRIME && clan->clan_type != CLAN_GUILD )
          continue;
        
        ch_printf( ch, "&Y%-24s &O%-10s\n",
                  clan->name, clan->leader );
        count++;
    }

    if ( !count )
    {
	set_char_color( AT_BLOOD, ch);
        send_to_char( "There are no organizations currently formed.\n", ch );
    }
    set_char_color( AT_WHITE, ch );
    
}

void do_planets( CHAR_DATA *ch, char *argument )
{
    PLANET_DATA *planet;
    int count = 0;
    AREA_DATA   *area;

    set_pager_color( AT_WHITE, ch );
    for ( planet = first_planet; planet; planet = planet->next )
    {
        pager_printf( ch, "&wPlanet: &G%-15s   &wGoverned By: &G%s %s\n", 
                   planet->name ,
                   planet->governed_by ? planet->governed_by->name : "",
                   IS_SET(planet->flags, PLANET_NOCAPTURE ) ? "(permanent)" : "" );
        pager_printf( ch, "&WValue: &O%-10.0f&W/&O%-10d   ", 
                   get_taxes(planet) , planet->base_value);
        pager_printf( ch, "&WPopulation: &O%-5d   &W Pop Support: &R%.1f\n", 
                   planet->population , planet->pop_support );
        if ( IS_IMMORTAL(ch) )
        {
          pager_printf( ch, "&WAreas: &G");
          for ( area = planet->first_area ; area ; area = area->next_on_planet )
             pager_printf( ch , "%s,  ", area->filename );
          pager_printf( ch, "\n" );
        }         
//        pager_printf( ch, "\n" );
        
        count++;
    }

    if ( !count )
    {
	set_char_color( AT_BLOOD, ch);
        send_to_char( "There are no planets currently formed.\n", ch );
    }
    
}

void do_orders( CHAR_DATA *ch, char *argument )
{
}

void do_guilds( CHAR_DATA *ch, char *argument)
{
}                                                                           

void do_shove( CHAR_DATA *ch, char *argument )
{
    std::string arg;
    std::string arg2;
    std::string argstr = argument;
    int exit_dir;
    EXIT_DATA *pexit;
    CHAR_DATA *victim;
    bool nogo;
    ROOM_INDEX_DATA *to_room;
    ROOM_INDEX_DATA *fromroom;
    SHIP_DATA *ship;    
    int chance;  

    argstr = one_argument( argstr, arg );
    argstr = one_argument( argstr, arg2 );

    
    if ( arg.empty() )
    {
	send_to_char( "Shove whom?\n", ch);
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n", ch);
	return;
    }

    if (victim == ch)
    {
	send_to_char("You shove yourself around, to no avail.\n", ch);
	return;
    }
    
    if ( (victim->position) != POS_STANDING )
    {
	act( AT_PLAIN, "$N isn't standing up.", ch, NULL, victim, TO_CHAR );
	return;
    }

    if ( arg2.empty() )
    {
	send_to_char( "Shove them in which direction?\n", ch);
	return;
    }

    exit_dir = get_dir( arg2 );
    if ( BV_IS_SET(victim->in_room->room_flags, ROOM_SAFE)
    &&  get_timer(victim, TIMER_SHOVEDRAG) <= 0)
    {
	send_to_char("That character cannot be shoved right now.\n", ch);
	return;
    }
    victim->position = POS_SHOVE;
    nogo = FALSE;
    if ((pexit = get_exit(ch->in_room, exit_dir)) == NULL )
    {
      if (!str_cmp( arg2, "in" ))
      {
        if ( argstr.empty() )
        {
          send_to_char( "Drag him into what?\n", ch );
          return;
        }

        if ( ( ship = ship_in_room( ch->in_room , argstr ) ) == NULL )
        {
          act( AT_PLAIN, "I see no $T here.", ch, NULL, argstr, TO_CHAR );
          return;
        }
        
	if ( BV_IS_SET( ch->act, ACT_MOUNTED ) )
   	{
          act( AT_PLAIN, "You can't go in there riding THAT.", ch, NULL, argstr, TO_CHAR );
          return;
   	} 

   	fromroom = ch->in_room;

        if ( ( to_room = get_room_index( ship->entrance ) ) != NULL )
   	{
   	   if ( ! ship->hatchopen )
   	   {
   	      send_to_char( "&RThe hatch is closed!\n", ch);
   	      return;
   	   }

           if ( to_room->tunnel > 0 )
           {
	        CHAR_DATA *ctmp;
	        int count = 0;

	       for ( ctmp = to_room->first_person; ctmp; ctmp = ctmp->next_in_room )
	       if ( count+2 >= to_room->tunnel )
	       {
                  send_to_char( "There is no room for you both in there.\n", ch );
		  return;
	       }
           }
            if ( ship->shipstate == SHIP_LAUNCH || ship->shipstate == SHIP_LAUNCH_2 )
            {
                 send_to_char("&rThat ship has already started launching!\n",ch);
                 return;
            }
            
            act( AT_PLAIN, "$n enters $T.", ch,
		NULL, ship->name , TO_ROOM );
	    act( AT_PLAIN, "You enter $T.", ch,
		NULL, ship->name , TO_CHAR );
   	    char_from_room( ch );
   	    char_to_room( ch , to_room );
   	    act( AT_PLAIN, "$n enters the ship.", ch,
		NULL, argstr , TO_ROOM );
            do_look( ch , "auto" );

            act( AT_PLAIN, "$n enters $T.", victim,
		NULL, ship->name , TO_ROOM );
	    act( AT_PLAIN, "You enter $T.", victim,
		NULL, ship->name , TO_CHAR );
   	    char_from_room( victim );
   	    char_to_room( victim , to_room );
   	    act( AT_PLAIN, "$n enters the ship.", victim,
		NULL, argstr , TO_ROOM );
            do_look( victim , "auto" );
            victim->position = POS_STANDING;            
	    return;
        }
        else
        {
          send_to_char("That ship has no entrance!\n", ch);
          return;
        }
      }  
      if (!str_cmp( arg2, "out" ))
      {
    	fromroom = ch->in_room;
    
	    if  ( (ship = ship_from_entrance(ch->game, fromroom->vnum)) == NULL )
	    {
	        send_to_char( "I see no exit here.\n" , ch );
	        return;
	    }   
        
	if ( BV_IS_SET( ch->act, ACT_MOUNTED ) )
   	{
          act( AT_PLAIN, "You can't go out there riding THAT.", ch, NULL, argstr, TO_CHAR );
          return;
   	} 

    if ( ship->lastdoc != ship->location )
    {
        send_to_char("&rMaybe you should wait until the ship lands.\n",ch);
        return;
    }
    
    if ( ship->shipstate != SHIP_LANDED && ship->shipstate != SHIP_DISABLED )
    {
        send_to_char("&rPlease wait till the ship is properly docked.\n",ch);
        return;
    }
    
    if ( ! ship->hatchopen )
    {
    	send_to_char("&RYou need to open the hatch first" , ch );
    	return;
    }
    
    if ( ( to_room = get_room_index( ship->location ) ) != NULL )
    {

           if ( to_room->tunnel > 0 )
           {
	        CHAR_DATA *ctmp;
	        int count = 0;

	       for ( ctmp = to_room->first_person; ctmp; ctmp = ctmp->next_in_room )
	       if ( count+2 >= to_room->tunnel )
	       {
                  send_to_char( "There is no room for you both in there.\n", ch );
		  return;
	       }
           }
            if ( ship->shipstate == SHIP_LAUNCH || ship->shipstate == SHIP_LAUNCH_2 )
            {
                 send_to_char("&rThat ship has already started launching!\n",ch);
                 return;
            }
            
            act( AT_PLAIN, "$n exits the ship.", ch,
		NULL, ship->name , TO_ROOM );
	    act( AT_PLAIN, "You exits the ship.", ch,
		NULL, ship->name , TO_CHAR );
   	    char_from_room( ch );
   	    char_to_room( ch , to_room );
   	    act( AT_PLAIN, "$n exits $T.", ch,
		NULL, ship->name , TO_ROOM );
            do_look( ch , "auto" );

            act( AT_PLAIN, "$n exits the ship.", victim,
		NULL, ship->name , TO_ROOM );
	    act( AT_PLAIN, "You exits the ship.", victim,
		NULL, ship->name , TO_CHAR );
   	    char_from_room( victim );
   	    char_to_room( victim , to_room );
   	    act( AT_PLAIN, "$n exits $T.", victim,
		NULL, ship->name , TO_ROOM );
            do_look( victim , "auto" );
	    victim->position = POS_STANDING;            
	    return;
        }
        else
        {
          send_to_char("That ship has no entrance!\n", ch);
          return;
        }
      }  
      nogo = TRUE;
    }
    else
    if ( IS_SET(pexit->exit_info, EX_CLOSED)
    && (!IS_AFFECTED(victim, AFF_PASS_DOOR)
    ||   IS_SET(pexit->exit_info, EX_NOPASSDOOR)) )
      nogo = TRUE;
    if ( nogo )
    {
	send_to_char( "There's no exit in that direction.\n", ch );
        victim->position = POS_STANDING;
	return;
    }
    to_room = pexit->to_room;

    if ( IS_NPC(victim) )
    {
	send_to_char("You can only shove player characters.\n", ch);
	return;
    }
    
    if (ch->in_room->area != to_room->area
    &&  !in_hard_range( victim, to_room->area ) )
    {
      send_to_char("That character cannot enter that area.\n", ch);
      victim->position = POS_STANDING;
      return;
    }

chance = 50;

/* Add 3 points to chance for every str point above 15, subtract for 
below 15 */

chance += ((get_curr_str(ch) - 15) * 3);

chance += (ch->top_level - victim->top_level);
 
if (chance < number_percent( ))
{
  send_to_char("You failed.\n", ch);
  victim->position = POS_STANDING;
  return;
}
    act( AT_ACTION, "You shove $M.", ch, NULL, victim, TO_CHAR );
    act( AT_ACTION, "$n shoves you.", ch, NULL, victim, TO_VICT );
    move_char( victim, get_exit(ch->in_room,exit_dir), 0);
    if ( !char_died(victim) )
      victim->position = POS_STANDING;
    WAIT_STATE(ch, 12);
    /* Remove protection from shove/drag if char shoves -- Blodkai */
    if ( BV_IS_SET(ch->in_room->room_flags, ROOM_SAFE)   
    &&   get_timer(ch, TIMER_SHOVEDRAG) <= 0 )
      add_timer( ch, TIMER_SHOVEDRAG, 10, NULL, 0 );
}

void do_drag( CHAR_DATA *ch, char *argument )
{
    std::string arg;
    std::string arg2;
    std::string argstr = argument;
    int exit_dir;
    CHAR_DATA *victim;
    EXIT_DATA *pexit;
    ROOM_INDEX_DATA *to_room;
    bool nogo;
    int chance;
   ROOM_INDEX_DATA *fromroom;
   SHIP_DATA *ship;

    argstr = one_argument( argstr, arg );
    argstr = one_argument( argstr, arg2 );

    if ( arg.empty() )
    {
	send_to_char( "Drag whom?\n", ch);
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n", ch);
	return;
    }

    if ( victim == ch )
    {
	send_to_char("You take yourself by the scruff of your neck, but go nowhere.\n", ch);
	return; 
    }

    if ( IS_NPC(victim) )
    {
	send_to_char("You can only drag player characters.\n", ch);
	return;
    }

    if ( victim->fighting )
    {
        send_to_char( "You try, but can't get close enough.\n", ch);
        return;
    }
          
    if ( arg2.empty() )
    {
	send_to_char( "Drag them in which direction?\n", ch);
	return;
    }

    exit_dir = get_dir( arg2 );

    if ( BV_IS_SET(victim->in_room->room_flags, ROOM_SAFE)
    &&   get_timer( victim, TIMER_SHOVEDRAG ) <= 0)
    {
	send_to_char("That character cannot be dragged right now.\n", ch);
	return;
    }

    nogo = FALSE;
    if ((pexit = get_exit(ch->in_room, exit_dir)) == NULL )
    {
      if (!str_cmp( arg2, "in" ))
      {
        if ( argstr.empty() )
        {
          send_to_char( "Drag him into what?\n", ch );
          return;
        }

        if ( ( ship = ship_in_room( ch->in_room , argstr ) ) == NULL )
        {
          act( AT_PLAIN, "I see no $T here.", ch, NULL, argstr, TO_CHAR );
          return;
        }
        
	if ( BV_IS_SET( ch->act, ACT_MOUNTED ) )
   	{
          act( AT_PLAIN, "You can't go in there riding THAT.", ch, NULL, argstr, TO_CHAR );
          return;
   	} 

   	fromroom = ch->in_room;

        if ( ( to_room = get_room_index( ship->entrance ) ) != NULL )
   	{
   	   if ( ! ship->hatchopen )
   	   {
   	      send_to_char( "&RThe hatch is closed!\n", ch);
   	      return;
   	   }

           if ( to_room->tunnel > 0 )
           {
	        CHAR_DATA *ctmp;
	        int count = 0;

	       for ( ctmp = to_room->first_person; ctmp; ctmp = ctmp->next_in_room )
	       if ( count+2 >= to_room->tunnel )
	       {
                  send_to_char( "There is no room for you both in there.\n", ch );
		  return;
	       }
           }
            if ( ship->shipstate == SHIP_LAUNCH || ship->shipstate == SHIP_LAUNCH_2 )
            {
                 send_to_char("&rThat ship has already started launching!\n",ch);
                 return;
            }
            
            act( AT_PLAIN, "$n enters $T.", ch,
		NULL, ship->name , TO_ROOM );
	    act( AT_PLAIN, "You enter $T.", ch,
		NULL, ship->name , TO_CHAR );
   	    char_from_room( ch );
   	    char_to_room( ch , to_room );
   	    act( AT_PLAIN, "$n enters the ship.", ch,
		NULL, argstr , TO_ROOM );
            do_look( ch , "auto" );

            act( AT_PLAIN, "$n enters $T.", victim,
		NULL, ship->name , TO_ROOM );
	    act( AT_PLAIN, "You enter $T.", victim,
		NULL, ship->name , TO_CHAR );
   	    char_from_room( victim );
   	    char_to_room( victim , to_room );
   	    act( AT_PLAIN, "$n enters the ship.", victim,
		NULL, argstr , TO_ROOM );
            do_look( victim , "auto" );
	    return;
        }
        else
        {
          send_to_char("That ship has no entrance!\n", ch);
          return;
        }
      }  
      if (!str_cmp( arg2, "out" ))
      {
    	fromroom = ch->in_room;
    
	    if  ( (ship = ship_from_entrance(ch->game, fromroom->vnum)) == NULL )
	    {
	        send_to_char( "I see no exit here.\n" , ch );
	        return;
	    }   
        
	if ( BV_IS_SET( ch->act, ACT_MOUNTED ) )
   	{
          act( AT_PLAIN, "You can't go out there riding THAT.", ch, NULL, argstr, TO_CHAR );
          return;
   	} 

    if ( ship->lastdoc != ship->location )
    {
        send_to_char("&rMaybe you should wait until the ship lands.\n",ch);
        return;
    }
    
    if ( ship->shipstate != SHIP_LANDED && ship->shipstate != SHIP_DISABLED )
    {
        send_to_char("&rPlease wait till the ship is properly docked.\n",ch);
        return;
    }
    
    if ( ! ship->hatchopen )
    {
    	send_to_char("&RYou need to open the hatch first" , ch );
    	return;
    }
    
    if ( ( to_room = get_room_index( ship->location ) ) != NULL )
    {

           if ( to_room->tunnel > 0 )
           {
	        CHAR_DATA *ctmp;
	        int count = 0;

	       for ( ctmp = to_room->first_person; ctmp; ctmp = ctmp->next_in_room )
	       if ( count+2 >= to_room->tunnel )
	       {
                  send_to_char( "There is no room for you both in there.\n", ch );
		  return;
	       }
           }
            if ( ship->shipstate == SHIP_LAUNCH || ship->shipstate == SHIP_LAUNCH_2 )
            {
                 send_to_char("&rThat ship has already started launching!\n",ch);
                 return;
            }
            
            act( AT_PLAIN, "$n exits the ship.", ch,
		NULL, ship->name , TO_ROOM );
	    act( AT_PLAIN, "You exits the ship.", ch,
		NULL, ship->name , TO_CHAR );
   	    char_from_room( ch );
   	    char_to_room( ch , to_room );
   	    act( AT_PLAIN, "$n exits $T.", ch,
		NULL, ship->name , TO_ROOM );
            do_look( ch , "auto" );

            act( AT_PLAIN, "$n exits the ship.", victim,
		NULL, ship->name , TO_ROOM );
	    act( AT_PLAIN, "You exits the ship.", victim,
		NULL, ship->name , TO_CHAR );
   	    char_from_room( victim );
   	    char_to_room( victim , to_room );
   	    act( AT_PLAIN, "$n exits $T.", victim,
		NULL, ship->name , TO_ROOM );
            do_look( victim , "auto" );
	    return;
        }
        else
        {
          send_to_char("That ship has no entrance!\n", ch);
          return;
        }
      }  
      nogo = TRUE;
    }
    else
    if ( IS_SET(pexit->exit_info, EX_CLOSED)
       && ( !( IS_AFFECTED( ch, AFF_PASS_DOOR ) && IS_AFFECTED( victim, AFF_PASS_DOOR ) )
       || IS_SET( pexit->exit_info, EX_NOPASSDOOR ) ) )
      nogo = TRUE;
    if ( nogo )
    {
	send_to_char( "There's no exit in that direction.\n", ch );
	return;
    }

    to_room = pexit->to_room;

    if (ch->in_room->area != to_room->area
    && !in_hard_range( victim, to_room->area ) )
    {
      send_to_char("That character cannot enter that area.\n", ch);
      return;
    }
    
    chance = 50;
    

if (chance < number_percent( ))
{
  send_to_char("You failed.\n", ch);
  return;
}
    if ( victim->position < POS_STANDING )
    {
	sh_int temp;

	temp = victim->position;
	victim->position = POS_DRAG;
	act( AT_ACTION, "You drag $M into the next room.", ch, NULL, victim, TO_CHAR ); 
	act( AT_ACTION, "$n grabs your hair and drags you.", ch, NULL, victim, TO_VICT ); 
	move_char( victim, get_exit(ch->in_room,exit_dir), 0);
	if ( !char_died(victim) )
	  victim->position = temp;
/* Move ch to the room too.. they are doing dragging - Scryn */
	move_char( ch, get_exit(ch->in_room,exit_dir), 0);
	WAIT_STATE(ch, 12);
	return;
    }
    send_to_char("You cannot do that to someone who is standing.\n", ch);
    return;
}

void do_enlist( CHAR_DATA *ch, char *argument )
{

	CLAN_DATA *clan;
        
	if ( IS_NPC(ch) || !ch->pcdata )
	{
	    send_to_char( "You can't do that.\n", ch );
	    return;
	}
        
	if ( ch->pcdata->clan )
	{
		ch_printf( ch , "You will have to resign from %s before you can join a new organization.\n", ch->pcdata->clan->name );
		return;
	}
        
	if ( ! BV_IS_SET( ch->in_room->room_flags , ROOM_RECRUIT ) )
	{
		send_to_char( "You don't seem to be in a recruitment office.\n", ch );
		return;
	}
	
	for ( clan = first_clan; clan; clan = clan->next )
	{
		if ( ( ch->in_room->vnum == clan->enlistroom1 ||
		       ch->in_room->vnum == clan->enlistroom2 ) )
		{
		  if( !str_cmp( clan->name, "The Empire" ) && ch->race != RACE_HUMAN && ch->race != RACE_DEFEL )
		  {
		    send_to_char( "&CThe recruiter says, 'You will need to find a sponsor to enlist'&R&w\n", ch );
		    return;
		  }
			BV_SET_BIT( ch->speaks, LANG_CLAN );
			++clan->members;
			STRFREE( ch->pcdata->clan_name );
			ch->pcdata->clan_name = QUICKLINK( clan->name );
			ch->pcdata->clan = clan;
			ch_printf( ch, "Welcome to %s.\n", clan->name );
			update_member( ch );
			save_clan ( clan );
			return;
		}
	}

	send_to_char( "They don't seem to be recruiting right now.\n", ch );
	return;

}

void do_resign( CHAR_DATA *ch, char *argument )
{
 
       	CLAN_DATA *clan;
        long lose_exp;    
            
        if ( IS_NPC(ch) || !ch->pcdata )
	{
	    send_to_char( "You can't do that.\n", ch );
	    return;
	}
        
        clan =  ch->pcdata->clan;
        
        if ( clan == NULL )
        {
	    send_to_char( "You have to join an organization before you can quit it.\n", ch );
	    return;
	}

       if ( !str_cmp( ch->name, ch->pcdata->clan->leader ) )
       {
           ch_printf( ch, "You can't resign from %s ... you are the leader!\n", clan->name );
           return;
       }
       
    if ( ch->speaking & LANG_CLAN )
      ch->speaking = LANG_COMMON;
    BV_REMOVE_BIT( ch->speaks, LANG_CLAN );
    --clan->members;
    if ( !str_cmp( ch->name, ch->pcdata->clan->number1 ) )
    {
	STRFREE( ch->pcdata->clan->number1 );
	ch->pcdata->clan->number1 = STRALLOC( "" );
    }
    if ( !str_cmp( ch->name, ch->pcdata->clan->number2 ) )
    {
	STRFREE( ch->pcdata->clan->number2 );
	ch->pcdata->clan->number2 = STRALLOC( "" );
    }
    remove_member( ch );
    ch->pcdata->clan = NULL;
    STRFREE(ch->pcdata->clan_name);
    ch->pcdata->clan_name = STRALLOC( "" );
    act( AT_MAGIC, "You resign your position in $t", ch, clan->name, NULL , TO_CHAR );
    
    lose_exp = UMAX( ch->experience[DIPLOMACY_ABILITY] - exp_level( ch->skill_level[DIPLOMACY_ABILITY]  ) , 0 );
    ch_printf( ch, "You lose %ld diplomacy experience.\n", lose_exp ); 
    ch->experience[DIPLOMACY_ABILITY] -= lose_exp; 

    STR_DISPOSE( ch->pcdata->bestowments );
    ch->pcdata->bestowments = str_dup("");

    save_char_obj( ch );	/* clan gets saved when pfile is saved */
    
    return;

}

void do_clan_withdraw( CHAR_DATA *ch, char *argument )
{
    CLAN_DATA *clan;
    long       amount;
    OBJ_DATA *obj;
    bool ch_comlink = FALSE;
    
    if ( IS_NPC( ch ) || !ch->pcdata->clan )
    {
	send_to_char( "You don't seem to belong to an organization to withdraw funds from...\n", ch );
	return;
    }

    if ( (ch->pcdata && ch->pcdata->bestowments
    &&    is_name("withdraw", ch->pcdata->bestowments))
    ||   !str_cmp( ch->name, ch->pcdata->clan->leader  ))
	;
    else
    {
   	send_to_char( "&RYour organization hasn't seen fit to bestow you with that ability." ,ch );
   	return;
    }

      if ( IS_IMMORTAL( ch ) )
          ch_comlink = TRUE;
      else
        for ( obj = ch->last_carrying; obj; obj = obj->prev_content )
        {
           if (obj->pIndexData->item_type == ITEM_COMLINK)
           ch_comlink = TRUE;
        }
    
      if ( !ch_comlink )
      {
        if (!ch->in_room || !BV_IS_SET(ch->in_room->room_flags, ROOM_BANK) )
        {
          send_to_char( "You must be in a bank or have a comlink to do that!\n", ch );
          return;
        }
      }


    clan = ch->pcdata->clan;
    
    amount = atoi( argument );
    
    if ( !amount )
    {
	send_to_char( "How much would you like to withdraw?\n", ch );
	return;
    }
    
    if ( amount > clan->funds )
    {
	ch_printf( ch,  "%s doesn't have that much!\n", clan->name );
	return;
    }
    
    if ( amount < 0 )
    {
	ch_printf( ch,  "Nice try...\n" );
	return;
    }
    
    ch_printf( ch,  "You withdraw %ld credits from %s's funds.\n", amount, clan->name );
    
    clan->funds -= amount;
    ch->gold += amount;
    save_clan ( clan );
            
}


void do_clan_donate( CHAR_DATA *ch, char *argument )
{
    CLAN_DATA *clan;
    long       amount;
    OBJ_DATA *obj;
    bool ch_comlink = FALSE;

    if ( IS_NPC( ch ) || !ch->pcdata->clan )
    {
	send_to_char( "You don't seem to belong to an organization to donate to...\n", ch );
	return;
    }

      if ( IS_IMMORTAL( ch ) )
          ch_comlink = TRUE;
      else
        for ( obj = ch->last_carrying; obj; obj = obj->prev_content )
        {
           if (obj->pIndexData->item_type == ITEM_COMLINK)
           ch_comlink = TRUE;
        }
    
      if ( !ch_comlink )
      {
        if (!ch->in_room || !BV_IS_SET(ch->in_room->room_flags, ROOM_BANK) )
        {
          send_to_char( "You must be in a bank or have a comlink to do that!\n", ch );
          return;
        }
      }


    clan = ch->pcdata->clan;
    
    amount = atoi( argument );
    
    if ( !amount )
    {
	send_to_char( "How much would you like to donate?\n", ch );
	return;
    }

    if ( amount < 0 )
    {
	ch_printf( ch,  "Nice try...\n" );
	return;
    }
    
    if ( amount > ch->gold )
    {
	send_to_char( "You don't have that much!\n", ch );
	return;
    }
    
    ch_printf( ch,  "You donate %ld credits to %s's funds.\n", amount, clan->name );
    
    clan->funds += amount;
    ch->gold -= amount;
    save_clan ( clan );
            
}

void do_newclan ( CHAR_DATA *ch , char *argument )
{
	send_to_char( "This command is being recycled to conserve thought.\n", ch );
	return;
}

void do_appoint ( CHAR_DATA *ch , char *argument )
{
    std::string arg;
    std::string argstr = argument;
    
    argstr = one_argument( argstr, arg );
    
    if ( IS_NPC( ch ) || !ch->pcdata )
      return;

    if ( !ch->pcdata->clan )
    {
	send_to_char( "Huh?\n", ch );
	return;
    }

    if (  str_cmp( ch->name, ch->pcdata->clan->leader  )  )
    {
	send_to_char( "Only your leader can do that!\n", ch );
	return;
    }

    if ( arg.empty() )
    {
	send_to_char( "Useage: appoint <name> < first | second >\n", ch );
	return;
    }
    
    if ( !str_cmp( argstr , "first" )  )
    {
         if ( ch->pcdata->clan->number1 && str_cmp( ch->pcdata->clan->number1 , "" ) )
         {
             send_to_char( "You already have someone in that position ... demote them first.\n", ch );
	     return;
	 }
         
         STRFREE( ch->pcdata->clan->number1 );
         ch->pcdata->clan->number1 = STRALLOC( arg );
    }        
    else if ( !str_cmp( argstr , "second" )  )
    {
         if ( ch->pcdata->clan->number2 && str_cmp( ch->pcdata->clan->number2 , "" ))
         {
             send_to_char( "You already have someone in that position ... demote them first.\n", ch );
	     return;
	 }
         
         STRFREE( ch->pcdata->clan->number2 );
         ch->pcdata->clan->number2 = STRALLOC( arg );
    }
    else do_appoint( ch , "" );
    save_clan ( ch->pcdata->clan );
        
}

void do_demote ( CHAR_DATA *ch , char *argument )
{
    
    if ( IS_NPC( ch ) || !ch->pcdata )
      return;

    if ( !ch->pcdata->clan )
    {
	send_to_char( "Huh?\n", ch );
	return;
    }

    if (  str_cmp( ch->name, ch->pcdata->clan->leader  )  )
    {
	send_to_char( "Only your leader can do that!\n", ch );
	return;
    }

    if ( argument[0] == '\0' )
    {
	send_to_char( "Demote who?\n", ch );
	return;
    }
    
    if ( !str_cmp( argument , ch->pcdata->clan->number1 )  )
    {
         send_to_char( "Player Demoted!", ch );
         
         STRFREE( ch->pcdata->clan->number1 );
         ch->pcdata->clan->number1 = STRALLOC( "" );
    }        
    else if ( !str_cmp( argument , ch->pcdata->clan->number2 )  )
    {
         send_to_char( "Player Demoted!", ch );
         
         STRFREE( ch->pcdata->clan->number2 );
         ch->pcdata->clan->number2 = STRALLOC( "" );
    }        
    else
    {
	send_to_char( "They seem to have been demoted already.\n", ch );
	return;
    }
    save_clan ( ch->pcdata->clan );
        
}

void do_capture ( CHAR_DATA *ch , char *argument )
{
   CLAN_DATA *clan;
   PLANET_DATA *planet;
   PLANET_DATA *cPlanet;
   float support = 0.0;
// int pCount = 0;   
   
   if ( !ch->in_room || !ch->in_room->area)
      return;   

   if ( IS_NPC(ch) || !ch->pcdata )
   {
       send_to_char ( "huh?\n" , ch );
       return;
   }
   
   if ( !ch->pcdata->clan )
   {
       send_to_char ( "You need to be a member of an organization to do that!\n" , ch );
       return;
   }
   
   if ( ch->pcdata->clan->mainclan )
      clan = ch->pcdata->clan->mainclan;
   else
      clan = ch->pcdata->clan;
      
   if ( clan->clan_type == CLAN_CRIME )
   {
       send_to_char ( "Crime fimilies aren't in the business of controlling worlds.\n" , ch );
       return;
   }

   if ( clan->clan_type == CLAN_GUILD )
   {
       send_to_char ( "Your organization serves a much greater purpose.\n" , ch );
       return;
   }

   if ( ( planet = ch->in_room->area->planet ) == NULL )
   {
       send_to_char ( "You must be on a planet to capture it.\n" , ch );
       return;
   }   
   
   if ( clan == planet->governed_by )
   {
       send_to_char ( "Your organization already controls this planet.\n" , ch );
       return;
   }
   
   if ( planet->spaceobject )
   {
       SHIP_DATA *ship;
       CLAN_DATA *sClan;
              
       for ( ship = first_ship ; ship ; ship = ship->next )
       {
	  if( !ship->spaceobject )
	    continue;
       	  if( ship->shipstate == SHIP_HYPERSPACE || ship->shipstate == SHIP_DISABLED )
       	    continue;
       	  if( !space_in_range_c( ship, planet->spaceobject ) )
       	    continue;
          sClan = get_clan(ship->owner);
          if ( !sClan ) 
             continue;
          if ( sClan->mainclan )
             sClan = sClan->mainclan;
          if ( sClan == planet->governed_by )
          {
             send_to_char ( "A planet cannot be captured while protected by orbiting spacecraft.\n" , ch );
             return;
          }
       }
   }

   if ( IS_SET( planet->flags, PLANET_NOCAPTURE ) )
   {
       send_to_char ( "This planet cannot be captured.\n" , ch);
       return;
   }

   if ( planet->pop_support > 0 )
   {
       send_to_char ( "The population is not in favour of changing leaders right now.\n" , ch );
       return;
   }
   
   for ( cPlanet = first_planet ; cPlanet ; cPlanet = cPlanet->next )
        if ( clan == cPlanet->governed_by )
        {
//          pCount++;
            support += cPlanet->pop_support;
        }
   
   if ( support < 0 )
   {
       send_to_char ( "There is not enough popular support for your organization!\nTry improving loyalty on the planets that you already control.\n" , ch );
       return;
   }
   
   planet->governed_by = clan;
   planet->pop_support = 50;
   
   echo_to_all( AT_RED , str_printf("%s has claimed the planet %s!", clan->name, planet->name ) , 0 );
   
   save_planet( planet );
      
   return; 
}

void do_empower ( CHAR_DATA *ch , char *argument )
{
    std::string arg;
    std::string arg2;
    std::string argstr = argument;
    CHAR_DATA *victim;
    CLAN_DATA *clan;

    if ( IS_NPC( ch ) || !ch->pcdata->clan )
    {
	send_to_char( "Huh?\n", ch );
	return;
    }

    clan = ch->pcdata->clan;

    if ( (ch->pcdata && ch->pcdata->bestowments
    &&    is_name("empower", ch->pcdata->bestowments))
    || !str_cmp( ch->name, clan->leader  ) )
	;
    else
    {
	send_to_char( "You clan hasn't seen fit to bestow that ability to you!\n", ch );
	return;
    }

    argstr = one_argument( argstr, arg );
    argstr = one_argument( argstr, arg2 );

    if ( arg.empty() )
    {
	send_to_char( "Empower whom to do what?\n", ch );
	return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
	send_to_char( "That player is not here.\n", ch);
	return;
    }

    if ( IS_NPC(victim) )
    {
	send_to_char( "Not on NPC's.\n", ch );
	return;
    }

    if ( victim == ch )
    {
	    send_to_char( "Nice try.\n", ch );
	    return;
    }
 
    if ( victim->pcdata->clan != ch->pcdata->clan )
    {
	    send_to_char( "This player does not belong to your clan!\n", ch );
	    return;
    }

    if (!victim->pcdata->bestowments)
      victim->pcdata->bestowments = str_dup("");

    if ( arg2.empty() || !str_cmp( arg2, "list" ) )
    {
        ch_printf( ch, "Current bestowed commands on %s: %s.\n",
                      victim->name, victim->pcdata->bestowments );
        ch_printf( ch, "Current salary on %s: %d.\n",
                      victim->name, victim->pcdata->salary );

        return;
    }
    
    if ( (victim->pcdata && victim->pcdata->bestowments
    &&    is_name(arg2, victim->pcdata->bestowments)) )
    {
	    send_to_char( "That player already has that power.\n", ch );
	    return;
    }

   if( str_cmp( ch->name, clan->leader ) && !str_cmp( ch->name, clan->number1 ) )
   {
      if( !is_name( arg2, ch->pcdata->bestowments ) )
      {
         send_to_char( "&RI don't think you're even allowed to do that.&W\n", ch );
         return;
      }
   }    
    

    if ( !str_cmp( arg2, "none" ) )
    {
        STR_DISPOSE( victim->pcdata->bestowments );
	    victim->pcdata->bestowments = str_dup("");
        ch_printf( ch, "Bestowments removed from %s.\n", victim->name );
        ch_printf( victim, "%s has removed your bestowed clan abilities.\n", ch->name );
        return;
    }
    else if ( !str_cmp( arg2, "pilot" ) )
    {
      STR_DISPOSE( victim->pcdata->bestowments );
      victim->pcdata->bestowments = str_dup( str_printf("%s %s", victim->pcdata->bestowments, arg2.c_str()) );
      ch_printf( victim, "%s has given you permission to fly clan ships.\n", 
             ch->name );
      send_to_char( "Ok, they now have the ability to fly clan ships.\n", ch );
    }
    else if ( !str_cmp( arg2, "withdraw" ) )
    {
      STR_DISPOSE( victim->pcdata->bestowments );
      victim->pcdata->bestowments = str_dup( str_printf("%s %s", victim->pcdata->bestowments, arg2.c_str()) );
      ch_printf( victim, "%s has given you permission to withdraw clan funds.\n", 
             ch->name );
      send_to_char( "Ok, they now have the ablitity to withdraw clan funds.\n", ch );
    }
    else if ( !str_cmp( arg2, "clanbuyship" ) )
    {
      STR_DISPOSE( victim->pcdata->bestowments );
      victim->pcdata->bestowments = str_dup( str_printf("%s %s", victim->pcdata->bestowments, arg2.c_str()) );
      ch_printf( victim, "%s has given you permission to buy clan ships.\n", 
             ch->name );
      send_to_char( "Ok, they now have the ablitity to use clanbuyship.\n", ch );
    }
    else if ( !str_cmp( arg2, "induct" ) )
    {
      STR_DISPOSE( victim->pcdata->bestowments );
      victim->pcdata->bestowments = str_dup( str_printf("%s %s", victim->pcdata->bestowments, arg2.c_str()) );
      ch_printf( victim, "%s has given you permission to induct new members.\n", 
             ch->name );
      send_to_char( "Ok, they now have the ablitity to induct new members.\n", ch );
    }
    else if ( !str_cmp( arg2, "empower" ) )
    {
      STR_DISPOSE( victim->pcdata->bestowments );
      victim->pcdata->bestowments = str_dup( str_printf("%s %s", victim->pcdata->bestowments, arg2.c_str()) );
      ch_printf( victim, "%s has given you permission to empower members.\n", 
             ch->name );
      send_to_char( "Ok, they now have the ablitity to empower members.\n", ch );
    }
    else if ( !str_cmp( arg2, "salary" ) )
    {
      STR_DISPOSE( victim->pcdata->bestowments );
      victim->pcdata->bestowments = str_dup( str_printf("%s %s", victim->pcdata->bestowments, arg2.c_str()) );
      ch_printf( victim, "%s has given you permission to assign salaries.\n", 
             ch->name );
      send_to_char( "Ok, they now have the ablitity to assign salaries.\n", ch );
    }
    else if ( !str_cmp( arg2, "roster" ) )
    {
      STR_DISPOSE( victim->pcdata->bestowments );
      victim->pcdata->bestowments = str_dup( str_printf("%s %s", victim->pcdata->bestowments, arg2.c_str()) );
      ch_printf( victim, "%s has given you permission to access the roster.\n", 
             ch->name );
      send_to_char( "Ok, they now have the ablitity to access the roster.\n", ch );
    }
    else
    {
      send_to_char( "Currently you may empower members with only the following:\n", ch ); 
      send_to_char( "\npilot:       ability to fly clan ships\n", ch );
      send_to_char(     "withdraw:    ability to withdraw clan funds\n", ch );
      send_to_char(     "clanbuyship: ability to buy clan ships\n", ch );    
      send_to_char(     "induct:      ability to induct new members\n", ch );    
      send_to_char(     "salary:      ability to assign salaries\n", ch );    
      send_to_char(     "roster:      ability to see the full clan roster\n", ch );
      send_to_char(     "none:        removes bestowed abilities\n", ch );    
    }
    
    save_char_obj( victim );	/* clan gets saved when pfile is saved */
    return;


}

void save_senate( )
{
/*
    BOUNTY_DATA *tbounty;
    FILE *fpout;
    char filename[256];
    
    SPRINTF( filename, "%s%s", SYSTEM_DIR, BOUNTY_LIST );
    fpout = fopen( filename, "w" );
    if ( !fpout )
    {
         bug( "FATAL: cannot open bounty.lst for writing!\n", 0 );
         return;
    }
    for ( tbounty = first_bounty; tbounty; tbounty = tbounty->next )
    {
        fprintf( fpout, "%s\n", tbounty->target );
        fprintf( fpout, "%ld\n", tbounty->amount );
    }
    fprintf( fpout, "$\n" );
    FCLOSE( fpout );
*/
}

void load_senate( GameContext *game )
{
    first_senator = NULL;
    last_senator = NULL;
/*
    FILE *fpList;
    char *target;
    char bountylist[256];
    BOUNTY_DATA *bounty;
    long int  amount;
     
    first_bounty = NULL;
    last_bounty	= NULL;

    first_disintigration = NULL;
    last_disintigration	= NULL;

    log_string( "Loading disintigrations..." );

    SPRINTF( bountylist, "%s%s", SYSTEM_DIR, DISINTIGRATION_LIST );
    FCLOSE( fpReserve );
    if ( ( fpList = fopen( bountylist, "r" ) ) == NULL )
    {
	perror( bountylist );
	exit( 1 );
    }

    for ( ; ; )
    {
        target = feof( fpList ) ? "$" : fread_word( fpList );
        if ( target[0] == '$' )
        break;                                  
	CREATE( bounty, BOUNTY_DATA, 1 );
    bounty->game = game;
        LINK( bounty, first_disintigration, last_disintigration, next, prev );
	bounty->target = STRALLOC(target);
	amount = fread_number( fpList );
	bounty->amount = amount;
    }
    FCLOSE( fpList );
    log_string(" Done bounties " );
    fpReserve = fopen( NULL_FILE, "r" );

    return;
*/
}

void do_senate( CHAR_DATA *ch, char *argument )
{
/*
    GOV_DATA *gov;
    int count = 0;
    
    set_char_color( AT_WHITE, ch );
    send_to_char( "\nGoverning Area                 Controlled By             Value\n", ch );
    for ( gov = first_gov; gov; gov = gov->next )
    {
        set_char_color( AT_YELLOW, ch );
        ch_printf( ch, "%-30s %-25s %-15ld\n", gov->name, gov->controlled_by , gov->value );
        count++;
    }

    if ( !count )
    {
        set_char_color( AT_GREY, ch );
        send_to_char( "There are no governments to capture at this time.\n", ch );
	return;
    }
*/
}

void do_addsenator( CHAR_DATA *ch , char *argument )
{   
/*
    GOVE_DATA *gov;
    
    CREATE( gov, GOV_DATA, 1 );
    gov->game = ch->game;
    LINK( gov, first_gov, last_gov, next, prev );

    gov->name		= STRALLOC( argument );
    gov->value          = atoi( arg2 );
    gov->vnum           = object;
    gov->controlled_by  = STRALLOC( "" );
        
    ch_printf( ch, "OK, making %s.\n", argument );
    save_govs();
*/
}

void do_remsenator( CHAR_DATA *ch , char *argument )

{
/*
	UNLINK( bounty, first_bounty, last_bounty, next, prev );
	STRFREE( bounty->target );
	DISPOSE( bounty );
	
	save_bounties();
*/
}

float get_taxes( PLANET_DATA *planet )
{
      float gain;
      
      gain = planet->base_value;
      gain += planet->base_value*planet->pop_support/100;
      gain += UMAX(0, planet->pop_support/10 * planet->population);
      
      return gain;
}

/*
    (link)->prev		= (insert)->prev;		
    if ( !(insert)->prev )					
      (first)			= (link);			
    else							
      (insert)->prev->next	= (link);			
    (insert)->prev		= (link);			
    (link)->next		= (insert);			
*/


void do_addsalary ( CHAR_DATA *ch , char *argument )
{
    std::string arg;
    std::string arg2;
    std::string argstr = argument;
    CHAR_DATA *victim;
    CLAN_DATA *clan;
    int salary;
//    bool info = FALSE;

    if ( IS_NPC( ch ) || !ch->pcdata->clan )
    {
	send_to_char( "Huh?\n", ch );
	return;
    }

    clan = ch->pcdata->clan;

    if ( (ch->pcdata && ch->pcdata->bestowments
    &&    is_name("salary", ch->pcdata->bestowments))
    || !str_cmp( ch->name, clan->leader  ) )
	;
    else
    {
	send_to_char( "You clan hasn't seen fit to bestow that ability to you!\n", ch );
	return;
    }

    argstr = one_argument( argstr, arg );
    argstr = one_argument( argstr, arg2 );
//    if ( arg2.empty() )
//      info = TRUE;
    
    
    salary = strtoi(arg2);

    if ( arg.empty() )
    {
	send_to_char( "Assign a salary to whom?\n", ch );
	return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
	send_to_char( "That player is not here.\n", ch);
	return;
    }

    if ( IS_NPC(victim) )
    {
	send_to_char( "Not on NPC's.\n", ch );
	return;
    }

    if ( victim == ch )
    {
	    send_to_char( "Nice try.\n", ch );
	    return;
    }
 
    if ( victim->pcdata->clan != ch->pcdata->clan )
    {
	    send_to_char( "This player does not belong to your clan!\n", ch );
	    return;
    }

    if ( salary < 0 )
    {
        ch_printf( ch, "Salary's must be positive!\n", victim->name );
        return;
    }

        victim->pcdata->salary = salary;
        ch_printf( ch, "%s has been assigned %d credits for a salary.\n", victim->name, salary );
        ch_printf( victim, "%s has give you a %d credit salary.\n", ch->name, salary );
        return;

}    

void show_members( CHAR_DATA *ch, const std::string& argument, const std::string& format )
{
     MEMBER_LIST	*members_list;
     MEMBER_DATA	*member;
     CLAN_DATA	*clan;
     int members = 0;
     for( members_list = first_member_list; members_list; members_list = members_list->next )
     {
         if( !str_cmp( members_list->name, argument ) )
             break;
     }

     if( !members_list )
         return;

     clan = get_clan( argument );

     if ( !clan  )
         return;

    pager_printf( ch, "\nMembers of %s\n", clan->name );
    pager_printf( ch, 
"------------------------------------------------------------\n" );
    pager_printf( ch, "Leader: %s\n", clan->leader );
    pager_printf( ch, "Number1: %s\n", clan->number1 );
    pager_printf( ch, "Number2: %s\n", clan->number2 );
    pager_printf( ch, "Spacecraft: %d  Vehicles: %d\n", clan->spacecraft, clan->vehicles );
    pager_printf( ch, 
"------------------------------------------------------------\n" );
    pager_printf( ch, "  Lvl         Name           Class Kills Deaths       Joined      Last On\n\n" );

     if( !format.empty() )
     {
         if( !str_cmp( format, "kills" )
             || !str_cmp( format, "deaths" )
             || !str_cmp( format, "alpha" ))
         {
             MS_DATA *insert = NULL;
             MS_DATA *sort;
             MS_DATA *first_member = NULL;
             MS_DATA *last_member = NULL;

             CREATE( sort, MS_DATA, 1 );
             sort->member = members_list->first_member;
             LINK( sort, first_member, last_member, next, prev );

             for( member = members_list->first_member->next; member; member = member->next )
             {
                 insert = NULL;
                 for( sort = first_member; sort; sort = sort->next )
                 {
                     if( !str_cmp( format, "kills" ))
                     {
                         if( member->kills > sort->member->kills )
                         {
                             CREATE( insert, MS_DATA, 1 );
                             insert->member = member;
                             INSERT( insert, sort, first_member, next, prev );
                             break;
                         }
                     }
                     else if( !str_cmp( format, "deaths" ))
                     {
                         if( member->deaths > sort->member->deaths )
                         {
                             CREATE( insert, MS_DATA, 1 );
                             insert->member = member;
                             INSERT( insert, sort, first_member, next, prev );
                             break;
                         }
                     }
                     else if( !str_cmp( format, "alpha" ))
                     {
                         if( strcmp( member->name, sort->member->name ) < 0 )
                         {
                             CREATE( insert, MS_DATA, 1 );
                             insert->member = member;
                             INSERT( insert, sort, first_member, next, prev );
                             break;
                         }
                     }

                 }
                 if( insert == NULL )
                 {
                     CREATE( insert, MS_DATA, 1 );
                     insert->member = member;
                     LINK( insert, first_member, last_member, next, prev );
                 }
             }

             for( sort = first_member; sort; sort = sort->next )
                 if( str_cmp( sort->member->name, clan->leader )
                     && str_cmp( sort->member->name, clan->number1 )
                     && str_cmp( sort->member->name, clan->number2 ) )
                 {
                     members++;
                     pager_printf( ch, "[%3d] %12s %15s %5d %6d %10s %10s\n",
                                   sort->member->level,
                                   capitalize(sort->member->name ).c_str(),
                                   ability_name[sort->member->plrclass],
                                   sort->member->kills,
                                   sort->member->deaths,
                                   sort->member->since,
                                   sort->member->laston );
		 }

         }

         for( member = members_list->first_member; member; member = member->next )
             if( !str_prefix( format, member->name ) )
             {
             	 members++;
                 pager_printf( ch, "[%3d] %12s %15s %5d %6d %10s %10s\n",
                               member->level,
                               capitalize(member->name ).c_str(),
                               ability_name[member->plrclass],
                               member->kills,
                               member->deaths,
                               member->since,
                               member->laston );
             }

     }
     else
     {

         for( member = members_list->first_member; member; member = member->next )
             if( str_cmp( member->name, clan->leader ) && str_cmp( member->name, clan->number1 )
                 && str_cmp( member->name, clan->number2 ) )
             {
             	 members++;
                 pager_printf( ch, "[%3d] %12s %15s %5d %6d %10s %10s\n", 
                 	       member->level,
                               capitalize(member->name).c_str(), 
                               ability_name[member->plrclass],
                               member->kills, 
                               member->deaths, 
                               member->since,
                               member->laston );
             }
     }

             pager_printf( ch, 
"------------------------------------------------------------\n" );
	     pager_printf( ch, "Total Members: %d\n", members);
             pager_printf( ch, 
"------------------------------------------------------------\n" );


}

void remove_member( CHAR_DATA *ch )
{
    MEMBER_LIST	*members_list;
    MEMBER_DATA	*member;

    if( !ch->pcdata )
        return;

    for( members_list = first_member_list; members_list; members_list = members_list->next )
    {
      if( !str_cmp( members_list->name, ch->pcdata->clan_name ) )
        for( member = members_list->first_member; member; member = member->next )
        {
          if( !str_cmp( member->name, ch->name ) )
          {
            UNLINK( member, members_list->first_member, members_list->last_member, next, prev );
            STRFREE( member->name );
            STRFREE( member->since );
            DISPOSE( member );
            save_member_list( members_list );
            break;
          }
        }
    }
}

void do_roster( CHAR_DATA *ch, char *argument )
{
     if( IS_NPC( ch ) || !ch->pcdata->clan
         || ( str_cmp(ch->name, ch->pcdata->clan->leader )
              && str_cmp(ch->name, ch->pcdata->clan->number1 )
              && str_cmp(ch->name, ch->pcdata->clan->number2 )
              && (!ch->pcdata || !ch->pcdata->bestowments
    || !is_name("roster", ch->pcdata->bestowments)) ) )
     {
         send_to_char( "Huh?\n", ch );
         return;
     }

     show_members( ch, ch->pcdata->clan->name, argument );
     return;

}
void do_members( CHAR_DATA *ch, char *argument )
{
     std::string arg1;
     std::string argstr = argument;
     argstr = one_argument( argstr, arg1 );

     if( argstr.empty() || arg1.empty() )
     {
         send_to_char( "Do what?\n", ch );
         return;
     }

     if( !str_cmp( arg1, "show" ) )
     {
         if( !str_cmp( argument, "all" ) )
         {
             MEMBER_LIST *members_list;
             for( members_list = first_member_list; members_list; members_list = members_list->next )
                 show_members( ch, members_list->name, NULL );
             return;
         }

         show_members( ch, argstr, NULL );
         return;
     }

     if( !str_cmp( arg1, "create" ) )
     {
         MEMBER_LIST *members_list;

         CREATE( members_list, MEMBER_LIST, 1 );
         members_list->game = ch->game;
         members_list->name = STRALLOC( argstr );
         LINK( members_list, first_member_list, last_member_list, next, prev );
         save_member_list( members_list );
         ch_printf( ch, "Member lists \"%s\" created.\n", argstr.c_str() );
         return;
     }

     if( !str_cmp( arg1, "delete" ) )
     {
         MEMBER_LIST *members_list;
         MEMBER_DATA *member;

         for( members_list = first_member_list; members_list; members_list = members_list->next )
             if( !str_cmp( argstr, members_list->name ) )
             {
                 while( members_list->first_member )
                 {
                     member = members_list->first_member;
                     STRFREE( member->name );
                     STRFREE( member->since );
                     UNLINK( member, members_list->first_member, members_list->last_member, next, prev );
                     DISPOSE( member );
                 }

                 STRFREE( members_list->name );
                 UNLINK( members_list, first_member_list, last_member_list, next, prev );
                 DISPOSE( members_list );
                 ch_printf( ch, "Member list \"%s\" destroyed.\n", argstr.c_str() );
                 return;
             }
         send_to_char( "No such list.\n", ch );
         return;
     }
}

void save_member_list( MEMBER_LIST *members_list )
{
     MEMBER_DATA	*member;
     FILE		*fp;
     CLAN_DATA    *clan;
     std::string         buf;

     if( !members_list )
     {
         bug( "save_member_list: NULL members_list" );
         return;
     }

     if( ( clan = get_clan( members_list->name )) == NULL )
     {
         bug( "save_member_list: no such clan: %s", members_list->name );
         return;
     }

     buf = str_printf("%s%s.mem", CLAN_DIR, clan->filename);

     if( ( fp = fopen( buf.c_str(), "w" ) ) == NULL )
     {
         bug( "Cannot open clan.mem file for writing", 0 );
         perror( buf.c_str() );
         return;
     }

     fprintf( fp, "Version       2\n" );
     fprintf( fp, "Name          %s~\n", members_list->name );
     for( member = members_list->first_member; member; member = member->next )
         fprintf( fp, "Member        %s %s %d %d %d %d %s\n", 
         member->name, member->since, member->kills, member->deaths, member->level, member->plrclass, member->laston);
     fprintf( fp, "End\n\n" );
     FCLOSE( fp );

}

bool load_member_list( GameContext *game, const std::string& filename )
{
     FILE *fp;
     std::string buf;
     MEMBER_LIST *members_list;
     MEMBER_DATA *member;
     int version;

     buf = str_printf("%s%s.mem", CLAN_DIR, filename.c_str());

     if( ( fp = fopen( buf.c_str(), "r" ) ) == NULL )
     {
         bug( "Cannot open member list for reading", 0 );
         return FALSE;
     }

     CREATE( members_list, MEMBER_LIST, 1 );
     members_list->game = game;

     for( ; ; )
     {
         char *word;

         word = fread_word( fp );

         if( !str_cmp( word, "Version" ) )
         {
             version = fread_number( fp );;
             continue;
         }
         else
         if( !str_cmp( word, "Name" ) )
         {
             members_list->name = fread_string( fp );
             continue;
         }
         else
         if( !str_cmp( word, "Member" ) )
         {
             CREATE( member, MEMBER_DATA, 1 );
             member->name = STRALLOC( fread_word( fp ) );
             member->since = STRALLOC( fread_word( fp ) );
             member->kills = fread_number( fp );
             member->deaths = fread_number( fp );
             member->level = fread_number( fp );
	     member->plrclass = fread_number( fp );
	     if ( version == 2 )
	       member->laston = STRALLOC( fread_word( fp ) );
	     
             LINK( member, members_list->first_member, members_list->last_member, next, prev );
             continue;
         }
         else
         if( !str_cmp( word, "End" ) )
         {
             LINK( members_list, first_member_list, last_member_list, next, prev );
             FCLOSE( fp );
             return TRUE;
         }
         else
         {
             bug( "load_members_list: bad section", 0 );
             FCLOSE( fp );
             return FALSE;
         }
     }

}

void update_member( CHAR_DATA *ch )
{
     MEMBER_LIST *members_list;
     MEMBER_DATA *member;
     struct tm *t = localtime(&current_time);

     if( IS_NPC( ch ) || IS_IMMORTAL(ch) || !ch->pcdata->clan )
         return;

     for( members_list = first_member_list; members_list; members_list = members_list->next )
         if( !str_cmp( members_list->name, ch->pcdata->clan_name ) )
         {
             for( member = members_list->first_member; member; member = member->next )
                 if ( !str_cmp( member->name, ch->name ) )
                 {
                     if( ch->pcdata->clan->clan_type == CLAN_PLAIN )
                     {
                         member->kills = ch->pcdata->pkills;
                         member->deaths = ch->pcdata->clones;
                     }
                     else
                     {
                         member->kills = ch->pcdata->pkills;
                         member->deaths = ch->pcdata->clones;
                     }
		             member->plrclass = ch->main_ability;
                     member->level = ch->top_level;
                     member->laston = STRALLOC( str_printf("[%02d|%02d|%04d]", t->tm_mon+1, t->tm_mday, t->tm_year+1900) );
                     gmcp_evt_char_status(ch);
                     save_member_list( members_list );
                     return;
                 }

             if( member == NULL )
             {
                 CREATE( member, MEMBER_DATA, 1 );
                 member->name = STRALLOC( ch->name );
                 member->level = ch->top_level;
		 member->plrclass = ch->main_ability;
                 member->since = STRALLOC( str_printf("[%02d|%02d|%04d]", t->tm_mon+1, t->tm_mday, t->tm_year+1900) );
                 member->laston = STRALLOC( str_printf("[%02d|%02d|%04d]", t->tm_mon+1, t->tm_mday, t->tm_year+1900) );
                 if( ch->pcdata->clan->clan_type == CLAN_PLAIN )
                 {
                     member->kills = ch->pcdata->pkills;
                     member->deaths = ch->pcdata->clones;
                 }
                 else
                 {
                     member->kills = ch->pcdata->pkills;
                     member->deaths = ch->pcdata->clones;
                 }
                 LINK( member, members_list->first_member, members_list->last_member, next, prev );
                 save_member_list( members_list );

             }

         }
    gmcp_evt_char_status(ch);

}
    
void do_clanfunds( CHAR_DATA *ch, char *argument )
{
  CLAN_DATA *clan;
  OBJ_DATA *obj;
  bool ch_comlink = FALSE;

  if ( IS_NPC( ch ) || !ch->pcdata->clan )
  {
    send_to_char("You don't seem to belong to an organization.\n",ch);
    return;
  }

  if ( IS_IMMORTAL( ch ) )
    ch_comlink = TRUE;
  else
    for ( obj = ch->last_carrying; obj; obj = obj->prev_content )
    {
      if (obj->pIndexData->item_type == ITEM_COMLINK)
        ch_comlink = TRUE;
    }
  
  if ( !ch_comlink )
  {
    if (!ch->in_room || !BV_IS_SET(ch->in_room->room_flags, ROOM_BANK) )
    {
      send_to_char( "You must be in a bank or have a comlink to do that!\n", ch );
      return;
    }
  }

  clan = ch->pcdata->clan;
  if ( clan->funds == 0 )
  {
    ch_printf(ch,"%s has no funds at its disposal.",clan->name);
    return;
  }
  ch_printf(ch,"%s has %ld credits at its disposal.\n",clan->name,clan->funds);
}
