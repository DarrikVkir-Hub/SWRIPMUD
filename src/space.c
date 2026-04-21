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
*		                Space Module    			   *   
****************************************************************************/

#include <math.h>
#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include "mud.h"
#include "space2.h"

SHIP_DATA * first_ship;
SHIP_DATA * last_ship;

MISSILE_DATA * first_missile;
MISSILE_DATA * last_missile;

SPACE_DATA * first_spaceobject;
SPACE_DATA * last_spaceobject;

int bus_pos =0;
int bus_planet =0;
int bus2_planet = 8;
int bus3_planet = 11;
int bus4_planet = 3;
int turbocar_stop =0;
int corus_shuttle =0;

int currrentalvnum = 6000; // increments as rentals are created

#define RENTALSTORAGEROOM 44 // Room where unused rentals are stored

#define DEGMULTI 149760

#define MAX_COORD 15000000
#define MAX_COORD_S 14000000
#define MAX_STATION    10
#define MAX_BUS_STOP 14

#define SHIPDAM_MINPLATFORM	100
#define SHIPDAM_MAXPLATFORM	250
#define SHIPDAM_MINCAP		100
#define SHIPDAM_MAXCAP		200
#define SHIPDAM_MINDEF		5
#define SHIPDAM_MAXDEF		10

#define STOP_PLANET     202
#define STOP_SHIPYARD   32015

#define SENATEPAD       32055
#define OUTERPAD        2064

struct	serin_shuttle
{
    char 		name	[20];	
    int			startingloc;
    int			cockpitvnum;
    int			planetloc;
    int		const  	bus_vnum [12];
    char * 	const   bus_stop [12];
};

/* New Serin system, makes it much easier to add new ones - Darrik Vequir, May, 01 */
/* MAX_BUS defined in mud.h */

struct serin_shuttle serin[MAX_BUS] =
{
{
  "Pluogus", 0, 32140, 0,
    { 201, 21943, 28613, 21100, 28038, 10500, 201 },
    { "Coruscant", "Corulag", "Kashyyyk", "Mon Calamari", "Gamorr", "Corellia", "Coruscant" }

},
{
  "Tocca", 0, 32410, 0,
    { 5550, 21943, 10500, 29001, 11800, 4100, 23082, 5550 },
    { "Ord Mantell", "Corulag", "Corellia", "Adari", "Roche", "Sullust", "Kuat", "Ord Mantell" }

},
{
  "Cassia", 0, 32425, 0,
    { 28247, 10500, 5200, 9729, 31872, 822, 28038, 10500, 28247 },
    { "Byss", "Corellia", "Af'el", "Uvena III", "Tatooine", "Ryloth", "Gamorr", "Corellia", "Byss" }

},
{
  "Siego", 0, 32435, 0,
    { 201, 10500, 6264, 11206, 2300, 4305, 28247, 201 },
    { "Coruscant", "Corellia", "Bespin", "Hoth", "Endor", "Wroona", "Byss", "Coruscant"}

},
{
  "Amose", 0, 32445, 0,
    { 2300, 6264, 4100, 31872, 9729, 28038, 21100, 5550, 2300 },
    { "Endor", "Bespin", "Sullust", "Tatooine", "Uvena III", "Gamorr", "Mon Calamari", "Ord Mantell", "Endor"}

},
{
  "Edena", 0, 32440, 0,
    { 28247, 201, 21943, 23082, 10500, 29001, 4305, 28247 },
    { "Byss", "Coruscant", "Corulag", "Kuat", "Corellia", "Adari", "Wroona", "Byss"}

},
{
  "Odani", 0, 32450, 0,
    { 4305, 11800, 28613, 5550, 23082, 10500, 29001, 4305 },
    { "Wroona", "Roche", "Kashyyyk", "Ord Mantell", "Kuat", "Corellia", "Adari", "Wroona"}

}
};


int     const   station_vnum [MAX_STATION] =
{
    215 , 216 , 217 , 218 , 219 , 220 , 221 ,222 , 223 , 224
};

char *  const   station_name [MAX_STATION] =
{
   "Menari Spaceport" , "Skydome Botanical Gardens" , "Grand Towers" ,
   "Grandis Mon Theater" , "Palace Station" , "Great Galactic Museum" ,
   "College Station" , "Holographic Zoo of Extinct Animals" ,
   "Dometown Station " , "Monument Plaza"
};
/*
int     const   bus_vnum [MAX_BUS_STOP] =
{
    201 ,  21100 , 29001 , 28038 , 31872 , 822 , 6264, 28613 , 3060 , 28247, 32297, 2065, 10500, 11206
};
int     const   bus_vnum2 [MAX_BUS_STOP] =
{
    201, 11206 ,  10500 , 2065 , 32297 , 28247 , 3060 , 28613 , 6264, 822 , 31872, 28038, 29001, 21100
};

char *  const   bus_stop [MAX_BUS_STOP+1] =
{
   "Coruscant",
   "Mon Calamari", "Adari", "Gamorr", "Tatooine" , "Ryloth", "Bespin",
   "Kashyyyk", "Endor", "Byss", "Cloning Facilities", "Kuat", "Corellia", "Hoth", "Coruscant"  // last should always be same as first 
};

char *  const bus_stoprev [MAX_BUS_STOP+1] =
{
   "Coruscant",
   "Hoth", "Corellia", "Kuat", "Cloning Facilities" , "Byss" , "Endor", "Kashyyyk",
   "Bespin", "Ryloth", "Tatooine", "Gamorr", "Adari", "Mon Calamari", "Coruscant"  // last should always be same as first
};
*/
RENTAL_DATA rentals[MAX_RENTALS] =
{
  {
  HOPPER_RENTAL, 6030
  },
  {
  TWING_RENTAL, 32200
  }
};

// TURBOLASER_TURRET, QUAD_TURRET, MAX_TURRET_TYPE, ION_TURRET, MISSILE_TURRET, TRACTOR_TURRET

struct turret_type turretstats[MAX_TURRET_TYPE] =
{
  { TURBOLASER_TURRET, 100, 250,
    "Turbo-laser Mount",
    "You are hit by turbolasers from %s!",
    "Turboasers fire from %s, hitting %s.",
    "The turbo-laser fires at %s, scoring a hit.",
    "Turbolasers fire from %s at you but miss.",
    "Turbolasers fire from %s at %s but miss.",
    "The turbo-laser fires at %s and misses."
  },
  { QUAD_TURRET, 25, 50,
    "Quad-cannon Mount",
    "You are hit by a quad cannon from %s!",
    "Laser cannon fire from %s, hitting %s.",
    "The laser cannon fires at %s, scoring a hit.",
    "Laser cannon fire from %s at flash by you but miss.",
    "Laser cannon fire from %s flash at %s but miss.",
    "The laser cannon fires at %s and misses."
  },
  { ION_TURRET, -25, -50,
    "Ion-cannon Mount",
    "You are hit by an ion cannon from %s!",
    "Ion cannon fire from %s, hitting %s.",
    "The ion cannon fires at %s, scoring a hit.",
    "Ion cannon fire from %s at flash by you but miss.",
    "Ion cannon fire from %s flash at %s but miss.",
    "The ion cannon fires at %s and misses."
  },
  { MISSILE_TURRET, 1, 1,
    "Missile Launcher Mount",
    "You are launched on by a missile turret from %s!",
    "A missile launcher fires from %s.",
    "The launcher fires at %s.",
    "Not used",
    "Not used",
    "The launcher attempts to launch on %s but misfires."
  },
  { TRACTOR_TURRET, 1, 1,
    "Tractor Beam Mount",
    "You are captured by a tractor beam from %s!",
    "%s captures %s with a tractor beam.",
    "The tractor beam locks on to %s.",
    "A tractor beam from %s attempts to capture you but loses lock.",
    "A tractor beam from %s attempts to capture %s, but loses lock.",
    "The tractor beam attempts to capture %s but loses lock."
  }
};


/* local routines */
void	fread_ship	args( ( SHIP_DATA *ship, FILE *fp ) );
void	write_ship_list	args( ( GameContext *game ) );
void    fread_spaceobject      args( ( SPACE_DATA *spaceobject, FILE *fp ) );
bool    load_spaceobject  args( ( const std::string& spaceobjectfile ) );
void    write_spaceobject_list args( ( GameContext *game ) );
void    resetship args( ( SHIP_DATA *ship ) );
void    approachland args( ( SHIP_DATA *ship, const std::string& arg ) );
void    landship args( ( SHIP_DATA *ship, const std::string& arg ) );
void    launchship args( ( SHIP_DATA *ship ) );
bool    land_bus args( ( SHIP_DATA *ship, int destination ) );
void    launch_bus args( ( SHIP_DATA *ship ) );
void    echo_to_room_dnr args( ( int ecolor , ROOM_INDEX_DATA *room ,  const std::string& argument ) );
ch_ret drive_ship( CHAR_DATA *ch, SHIP_DATA *ship, EXIT_DATA  *exit , int fall );
bool is_facing( SHIP_DATA *ship , SHIP_DATA *target );
void sound_to_ship( SHIP_DATA *ship , const std::string& argument );
void modtrainer( SHIP_DATA *ship, sh_int shipclass );
void makedebris( SHIP_DATA *ship );
bool space_in_range_h( SHIP_DATA *ship, SPACE_DATA *space);
void dumpship( SHIP_DATA *ship, int destination );
void shipdelete( SHIP_DATA *ship, bool shiplist );
void storeship( SHIP_DATA *ship );
/* from comm.c */

ROOM_INDEX_DATA *generate_exit( ROOM_INDEX_DATA *in_room, EXIT_DATA **pexit );


void echo_to_room_dnr ( int ecolor , ROOM_INDEX_DATA *room ,  const std::string& argument )
{
    CHAR_DATA *vic;

    if ( room == NULL )
    	return;

    for ( vic = room->first_person; vic; vic = vic->next_in_room )
    {
	set_char_color( ecolor, vic );
	send_to_char( argument, vic );
    }
}


bool  land_bus( SHIP_DATA *ship, int destination )
{
    
    if ( !ship_to_room( ship , destination ) )
    {
       return FALSE;
    }
    echo_to_ship( AT_YELLOW , ship , "You feel a slight thud as the ship sets down on the ground.");
    ship->location = destination;
    ship->lastdoc = ship->location;
    ship->shipstate = SHIP_LANDED;
    if (ship->spaceobject)
        ship_from_spaceobject( ship, ship->spaceobject );
    echo_to_room( AT_YELLOW , get_room_index(ship->location) , str_printf("%s lands on the platform.", ship->name) );
    echo_to_room( AT_YELLOW , get_room_index(ship->location) , str_printf("The hatch on %s opens." , ship->name) );
    echo_to_room( AT_YELLOW , get_room_index(ship->entrance) , "The hatch opens." );
    ship->hatchopen = TRUE;
    sound_to_room( get_room_index(ship->entrance) , "!!SOUND(door)" );
    sound_to_room( get_room_index(ship->location) , "!!SOUND(door)" );
    return TRUE;
}

void    launch_bus( SHIP_DATA *ship )
{

      sound_to_room( get_room_index(ship->entrance) , "!!SOUND(door)" );
       sound_to_room( get_room_index(ship->location) , "!!SOUND(door)" );
      echo_to_room( AT_YELLOW , get_room_index(ship->location) , str_printf("The hatch on %s closes and it begins to launch." , ship->name) );
      echo_to_room( AT_YELLOW , get_room_index(ship->entrance) , "The hatch slides shut." );
      ship->hatchopen = FALSE;
      extract_ship( ship );
      echo_to_ship( AT_YELLOW , ship , "The ship begins to launch.");
      ship->location = 0;
      ship->shipstate = SHIP_READY;
}


bool is_bus_stop( int vnum )
{
  int bus, stop;

  for ( bus = 0; bus < MAX_BUS; bus++ )
    for ( stop = 0; stop < 12 && serin[bus].bus_vnum[stop] != 0; stop++ )    
      if ( vnum == serin[bus].bus_vnum[stop] )
        return TRUE;
  return FALSE;
}


void update_traffic( GameContext *game )
{
    SHIP_DATA *shuttle, *senate;
    SHIP_DATA *turbocar;

    shuttle = ship_from_cockpit( game, ROOM_CORUSCANT_SHUTTLE );
    senate = ship_from_cockpit( game, ROOM_SENATE_SHUTTLE );

    if (!shuttle->game)       shuttle->game = game;
    if (!senate->game)        senate->game = game;

    if ( senate != NULL && shuttle != NULL )
    {
        switch (corus_shuttle)
        {
             default:
                corus_shuttle++;
                break;

             case 0:
                land_bus( shuttle , STOP_PLANET );
                land_bus( senate , SENATEPAD );
                corus_shuttle++;
                echo_to_ship( AT_CYAN , shuttle , "Welcome to Menari Spaceport." );
                echo_to_ship( AT_CYAN , senate , "Welcome to Kuat Shipyard." );
                break;

             case 4:
                launch_bus( shuttle );
                launch_bus( senate );
                corus_shuttle++;
                break;

             case 5:
                land_bus( shuttle , STOP_SHIPYARD );
                land_bus( senate , OUTERPAD );
                echo_to_ship( AT_CYAN , shuttle , "Welcome to Coruscant Shipyard." );
                echo_to_ship( AT_CYAN , senate , "Welcome to the planet of Kuat." );
                corus_shuttle++;
                break;
                
             case 9:
                launch_bus( shuttle );
                launch_bus( senate );
                corus_shuttle++;
                break;

        }
        
        if ( corus_shuttle >= 10 )
              corus_shuttle = 0;
    }
    
    turbocar = ship_from_cockpit( game, ROOM_CORUSCANT_TURBOCAR );
    if ( turbocar != NULL )
    {
        if (!turbocar->game)
          turbocar->game = game;
      	echo_to_room( AT_YELLOW , get_room_index(turbocar->location) , ("The turbocar doors close and it speeds out of the station.") );
      	extract_ship( turbocar );
      	turbocar->location = 0;
       	ship_to_room( turbocar , station_vnum[turbocar_stop] );
       	echo_to_ship( AT_YELLOW , turbocar , "The turbocar makes a quick journey to the next station."); 
       	turbocar->location = station_vnum[turbocar_stop];
       	turbocar->lastdoc = turbocar->location;
       	turbocar->shipstate = SHIP_LANDED;
       	if (turbocar->spaceobject)
          ship_from_spaceobject( turbocar, turbocar->spaceobject );  
    	echo_to_room( AT_YELLOW , get_room_index(turbocar->location) , ("A turbocar pulls into the platform and the doors slide open.") );
    	echo_to_ship( AT_CYAN , turbocar , str_printf("Welcome to %s." , station_name[turbocar_stop]) );
        turbocar->hatchopen = TRUE;
        
        turbocar_stop++;
        if ( turbocar_stop >= MAX_STATION )
           turbocar_stop = 0;
    }
       
}

void update_bus( GameContext *game )
{
    SHIP_DATA * ship[MAX_BUS];
/*    SHIP_DATA *ship2;
    SHIP_DATA *ship3;
    SHIP_DATA *ship4;
*/
    SHIP_DATA *target;
    int        destination, bus;
    std::string buf;

   for( bus = 0; bus < MAX_BUS; bus++ )
   {
     ship[bus] = ship_from_cockpit( game, serin[bus].cockpitvnum );
     if (ship[bus] && !ship[bus]->game)
       ship[bus]->game = game;
   }

    switch (bus_pos)
    {

       case 0:
            for( bus = 0; bus < MAX_BUS; bus++ )
            {
              if( ship_from_cockpit( game, serin[bus].cockpitvnum ) == NULL )
                continue;
              target = ship_from_hanger( game, serin[bus].bus_vnum[serin[bus].planetloc] );
              if ( target != NULL && !target->spaceobject )
              {
                echo_to_ship( AT_CYAN , ship[bus] , str_printf("An electronic voice says, 'Cannot land at %s ... it seems to have disappeared.'", serin[bus].bus_stop[serin[bus].planetloc]) );
                bus_pos = 5;
              }
            }

            bus_pos++;
            break;
       case 6:

	    for( bus = 0; bus < MAX_BUS; bus++ )
	    {
        if( ship_from_cockpit( game, serin[bus].cockpitvnum ) == NULL )
          continue;
	      launch_bus( ship[bus] );
	    }
            bus_pos++;
            break;

       case 7:

       for( bus = 0; bus < MAX_BUS; bus++ )
	    {
         if( ship_from_cockpit( game, serin[bus].cockpitvnum ) == NULL )
              continue;
    	
         echo_to_ship( AT_YELLOW , ship[bus] , "The ship lurches slightly as it makes the jump to lightspeed.");
      }
            bus_pos++;
            break;

       case 9:

	    for( bus = 0; bus < MAX_BUS; bus++ )
	    {
        if( ship_from_cockpit( game, serin[bus].cockpitvnum ) == NULL )
              continue;
        echo_to_ship( AT_YELLOW , ship[bus] , "The ship lurches slightly as it comes out of hyperspace.");
	    }
	      
            bus_pos++;
            break;

       case 1:

	    for( bus = 0; bus < MAX_BUS; bus++ )
          {
            if( ship_from_cockpit( game, serin[bus].cockpitvnum ) == NULL )
              continue;
            destination = serin[bus].bus_vnum[serin[bus].planetloc];
            if ( !land_bus( ship[bus], destination ) )
            {
               echo_to_ship( AT_CYAN , ship[bus] , str_printf("An electronic voice says, 'Oh My, %s seems to have disappeared.'" , serin[bus].bus_stop[serin[bus].planetloc]) );
               echo_to_ship( AT_CYAN , ship[bus] , "An electronic voice says, 'Alderaan was bad enough... Landing aborted.'");
            }
            else
            {
               echo_to_ship( AT_CYAN , ship[bus] , str_printf("An electronic voice says, 'Welcome to %s'" , serin[bus].bus_stop[serin[bus].planetloc]) );
               echo_to_ship( AT_CYAN , ship[bus] , "It continues, 'Please exit through the main ramp. Enjoy your stay.'");
            }
          }

            bus_pos++;
            break;

       case 5:
         for( bus = 0; bus < MAX_BUS; bus++)
         {
            if( ship_from_cockpit( game, serin[bus].cockpitvnum ) == NULL )
              continue;

            buf = str_printf("It continues, 'Next stop, %s'" , serin[bus].bus_stop[serin[bus].planetloc+1]);
            echo_to_ship( AT_CYAN , ship[bus] , "An electronic voice says, 'Preparing for launch.'");
            echo_to_ship( AT_CYAN , ship[bus] , buf);
         }
            bus_pos++;
            break;

       default:
            bus_pos++;
            break;
    }

    if ( bus_pos >= 10 )
    {
        bus_pos = 0;
	for( bus = 0; bus < MAX_BUS; bus++ )
	{
	  serin[bus].planetloc++;
          if( serin[bus].bus_vnum[serin[bus].planetloc] == serin[bus].bus_vnum[0] )
            serin[bus].planetloc = 0;
        }

    }
       
}
/*
Current Space Update System:

Move ship function
    Spaceobject loop 
        Loops through all spaceobjects, and moves them through space per their velocity

    Missile loop
        Loops through all missiles, and moves them through space per their velocity
            If the missile is in range of their target, explodes, deals damage, etc

    Ship loop (1) (Position updates)
        Loops through all ships
            Changes speed according to their goalspeed and accel
            Moves ships per their speed in realspace
            Checks to see if the ship is tractored, and moves it if tractored by something bigger, or moves the tractoree if they are equal or smaller 

            Loops through all spaceobjects
                Checks for a sun nearby and adjust the ship to evade
                Enters orbit if close enough to a planet

    Ship loop (2) (Hyperspace updates)  - Handles gravity well interaction and hyperspace exit, needs to happen only after all ships have been updated in the previous loop
        Loops through all ships
            If in hyperspace, adjusts position in hyperspace

            Loops through all spaceobjects
                If too close to a spaceobject, drops out of hyperspace
            
            Loops through all ships
                If too close to a ship with a gravity projector, drops out of hyperspace

            If in hyperspace and reached their destination, drop out of hyperspace

            If doing a tracking hyperspace iteration (doing short jumps repeatedly) and it's time for a pause, drop out of hyperspace
                Loop through all spaceobjects, and set the first one in range to the ship's current location.
                Recalculate the next iteration of the jump
            
            Check for remaining distance, display or alarm if short
        
            If ship is docked to another, force all coordinates to match the ship's

Recharge ship function

    Ship loop
        Recharge 1 laser and 1 ion (removing corresponding energy)

        Loop through ship's turrets
            Recharge turret, removing energy
        
        If docked to a platform, recharge energy

        Take a step towards recharging the missile launcher (takes three steps to fully recharge)

        If ship is on autopilot:
            If ship is in space and has a target
                Take a round of shots, choosing weapons according to shield status

        If ship is on autopilot:  (Yes, this is the same thing a second time - not sure why)
            If ship is in space and has a target
                Take a round of shots, choosing weapons according to shield status

Space Update function

    Ship Loop (1)
        Deduct energy if disabled or grav projector is on
        Decrement chaff_released by 1
        Increment count on the short hyperjump tracking
        Update manuvering and launching counts (takes several to complete)
        Increase shield if autorecharge, removing energy
        Increase shield if on autopilot, removing energy
        Cut shields if out of energy

        If in a system
            Show ship 'prompt' 
        
            Loop through all ships
                Show proximity alert

            Loop through all spaceobjects
                Show proximity alerts

        Check to see if the ship's target is still there, clear if not.
        Loop through the ship's turrets, check if the turret's target is still there, clear if not

        Warn if fuel is too low

    Ship loop (2)       // Have a linked list with ships on autopilot?  Move autotrack to realspace function?
        If autofly
            If target is no longer in range, return to idle

        If autotrack is on, adjust course to face the target, or evade if too close

        If autofly and in system
            if  has a target
                Set target's target to ship if target is also on autofly
                Loop through all ships
                    If any ship matches the owner of the main ship, set their target to your target
                if in range and launcher is ready, fire an appropriate projectile
            else accel and speed to 0
*/

static void move_spaceobjects(GameContext *game)
{
    SPACE_DATA *spaceobj;
    float dx, dy, dz, change;

    for( spaceobj = first_spaceobject; spaceobj; spaceobj = spaceobj->next )
    {
        if ( !spaceobj->game )
          spaceobj->game = game;

        if ( spaceobj->speed > 0 )
        {
          
            change = sqrt( spaceobj->hx*spaceobj->hx + spaceobj->hy*spaceobj->hy + spaceobj->hz*spaceobj->hz ); 

            if (change > 0)
            {
                dx = spaceobj->hx/change;     
                dy = spaceobj->hy/change;
                dz = spaceobj->hz/change;
                spaceobj->xpos += (dx * spaceobj->speed/5);
                spaceobj->ypos += (dy * spaceobj->speed/5);
                spaceobj->zpos += (dz * spaceobj->speed/5);
            }
           
        } 

        if( spaceobj->xpos > MAX_COORD )
          spaceobj->xpos = -MAX_COORD_S;
        if( spaceobj->ypos > MAX_COORD )
          spaceobj->ypos = -MAX_COORD_S;
        if( spaceobj->zpos > MAX_COORD )
          spaceobj->zpos = -MAX_COORD_S;
        if( spaceobj->xpos < -MAX_COORD )
          spaceobj->xpos = MAX_COORD_S;
        if( spaceobj->ypos < -MAX_COORD )
          spaceobj->ypos = MAX_COORD_S;
        if( spaceobj->zpos < -MAX_COORD )
          spaceobj->zpos = MAX_COORD_S;

    }
}

static void move_missiles( GameContext *game )
{
    MISSILE_DATA *missile;
    MISSILE_DATA *m_next;
    SHIP_DATA *ship;
    SHIP_DATA *target;
    std::string buf;
    CHAR_DATA *ch;
    bool ch_found = FALSE;    

    for ( missile = first_missile; missile; missile = m_next )
    {
        ch_found = FALSE;
        m_next = missile->next;

        if ( !missile->game )
            missile->game = game;

        if ( !missile->fired_from || !missile->target )
        {
            extract_missile( missile );
            continue;
        }

        ship = missile->fired_from;
        target = missile->target;

        if ( !ship || !target )
        {
            extract_missile( missile );
            continue;
        }
              
        if ( target->spaceobject && (missile_in_range( ship, missile )) )
        {
            if ( missile->mx < target->vx ) 
                missile->mx += UMIN( missile->speed/5 , target->vx - missile->mx );
            else if ( missile->mx > target->vx ) 
                missile->mx -= UMIN( missile->speed/5 , missile->mx - target->vx );  
            if ( missile->my < target->vy )
                missile->my += UMIN( missile->speed/5 , target->vy - missile->my );
            else if ( missile->my > target->vy ) 
                missile->my -= UMIN( missile->speed/5 , missile->my - target->vy );
            if ( missile->mz < target->vz )
                missile->mz += UMIN( missile->speed/5 , target->vz - missile->mz );
            else if ( missile->mz > target->vz ) 
                missile->mz -= UMIN( missile->speed/5 , missile->mz - target->vz );  
          
            if ( abs( (int) ( missile->mx-target->vx )) <= 20 
            && abs( (int) ( missile->my-target->vy )) <= 20 
            && abs((int) ( missile->mz-target->vz )) <= 20 )
            {  
                if ( target->chaff_released <= 0)
                { 
                    echo_to_room( AT_YELLOW , get_room_index(ship->gunseat), "Your missile hits its target dead on!" );
                    echo_to_cockpit( AT_BLOOD, target, "The ship is hit by a missile.");
                    echo_to_ship( AT_RED , target , "A loud explosion shakes thee ship violently!" );
                    buf = str_printf("You see a small explosion as %s is hit by a missile" , target->name );
                    echo_to_system( AT_ORANGE , target , buf , ship );
                    for ( ch = first_char; ch; ch = ch->next )
                    {
                        if ( !IS_NPC( ch ) && nifty_is_name( missile->fired_by, ch->name ) )
                        {
                          ch_found = TRUE;
                          damage_ship_ch( target , 30+missile->missiletype*missile->missiletype*30 , 
                              50+missile->missiletype*missile->missiletype*missile->missiletype*50 , ch );
                        }
                    }
                    if ( !ch_found )
                        damage_ship( target , ship, 20+missile->missiletype*missile->missiletype*20 , 
                            30+missile->missiletype*missile->missiletype*ship->missiletype*30 );   
                    extract_missile( missile );
                }
                else
                {
                    echo_to_room( AT_YELLOW , get_room_index(ship->gunseat), "Your missile explodes harmlessly in a cloud of chaff!" );
                    echo_to_cockpit( AT_YELLOW, target, "A missile explodes in your chaff.");
                    extract_missile( missile );
                }
                continue;
            }
            else
            {
                missile->age++;
                if (missile->age >= 50)
                {
                    extract_missile( missile );
                    continue;
                }
            }
        }
        else
        { 
            missile->age++;
            if (missile->age >= 50)
              extract_missile( missile );
        }

   }
   
}

static void update_ships_realspace(GameContext *game)
{
    SHIP_DATA *ship;
    SPACE_DATA *spaceobj;
    float dx, dy, dz, change;
    std::string buf;

    for ( ship = first_ship; ship; ship = ship->next )
    {
        if ( !ship->game )
            ship->game = game;
        
        if ( ship->shipstate != SHIP_HYPERSPACE && ship->accel && ship->currspeed != ship->goalspeed )
        {
            if( ship->goalspeed > ship->currspeed )
            {
                ship->currspeed += ship->accel;
                if ( ship->currspeed > ship->goalspeed )
                  ship->currspeed = ship->goalspeed;
            }
            else if( ship->goalspeed < ship->currspeed )
            {
                ship->currspeed -= ship->accel;
                if ( ship->currspeed < ship->goalspeed )
                  ship->currspeed = ship->goalspeed;
            }
            else if ( ship->currspeed <= 0 && ship->goalspeed <= 0 && ship->accel )
            {
                ship->accel = 0; 
                echo_to_cockpit( AT_YELLOW, ship, "Your computer beeps as the ship reaches a stop.");
                buf = str_printf("%s slows to a stop." , ship->name);
                echo_to_system( AT_ORANGE , ship , buf , NULL );
            }
            else if ( ship->currspeed >= ship->realspeed && ship->goalspeed >= ship->realspeed && ship->accel )
            {
                ship->accel = 0; 
                echo_to_cockpit( AT_YELLOW, ship, "Your computer beeps as the ship reaches max speed.");
                buf = str_printf("%s steadies its speed." , ship->name);
                echo_to_system( AT_ORANGE , ship , buf , NULL );
            }
            else if( ship->goalspeed == ship->currspeed && ship->accel )
            {
                ship->accel = 0; 
                echo_to_cockpit( AT_YELLOW, ship, "Your computer beeps as the ship reaches designated speed.");
                buf = str_printf("%s steadies its speed." , ship->name );
                echo_to_system( AT_ORANGE , ship , buf , NULL );
            }
        }

        if ( !ship->spaceobject )
            continue;
            
        if( ship->shipstate == SHIP_LANDED && ship->spaceobject )
            space_set_shipstate( ship, SHIP_READY );

        if ( ship->currspeed > 0 && ship->shipstate != SHIP_LAND && ship->shipstate != SHIP_LAND_2)
        {
          
            change = sqrt( ship->hx*ship->hx + ship->hy*ship->hy + ship->hz*ship->hz ); 

            if (change > 0)
            {
              dx = ship->hx/change;     
              dy = ship->hy/change;
              dz = ship->hz/change;
              ship->vx += (dx * ship->currspeed/5);
              ship->vy += (dy * ship->currspeed/5);
              ship->vz += (dz * ship->currspeed/5);
            }
          
        } 
        space_validate_tractor_links( NULL, ship, false );
        if ( ship->tractoredby && ( ship->tractoredby->shipclass <= ship->shipclass || ship->shipclass == SHIP_DEBRIS ) )
        {
            ship->tractoredby->currspeed = (ship->tractoredby->mod->tractorbeam)/4;
            ship->tractoredby->currspeed = UMAX( ship->tractoredby->currspeed, ship->currspeed );
            ship->tractoredby->hx = ship->vx - ship->tractoredby->vx;
            ship->tractoredby->hy = ship->vy - ship->tractoredby->vy;
            ship->tractoredby->hz = ship->vz - ship->tractoredby->vz;
        } 

        if ( ship->tractoredby && ship->tractoredby->shipclass > ship->shipclass )
        {
            ship->currspeed = ship->tractoredby->mod->tractorbeam/4;
            ship->currspeed = UMAX( ship->currspeed, ship->tractoredby->currspeed );
            ship->hx = ship->tractoredby->vx - ship->vx;
            ship->hy = ship->tractoredby->vy - ship->vy;
            ship->hz = ship->tractoredby->vz - ship->vz;
        }    


        if ( ship->tractoredby )
        {
            if ( abs( (int) ( ship->vx - ship->tractoredby->vx )) < 10 &&
                      abs( (int) ( ship->vy - ship->tractoredby->vy )) < 10 &&
                      abs( (int) ( ship->vz - ship->tractoredby->vz )) < 10 )
            {
                if ( ship->shipclass < ship->tractoredby->shipclass )
                {
                  ship->vx = ship->tractoredby->vx;
                  ship->vy = ship->tractoredby->vy;
                  ship->vz = ship->tractoredby->vz;
                }
                else if ( ship->shipclass >= ship->tractoredby->shipclass )	
                {
                  ship->tractoredby->vx = ship->vx;
                  ship->tractoredby->vy = ship->vy;
                  ship->tractoredby->vz = ship->vz;
                }
            }
        }


        if ( autofly(ship) )
          continue;
        
        for( spaceobj = first_spaceobject; spaceobj; spaceobj = spaceobj->next )
        {           
            if ( spaceobj->type == SPACE_SUN && spaceobj->name && strcmp(spaceobj->name,"") &&
                      abs( (int) ( ship->vx - spaceobj->xpos )) < 10 &&
                      abs( (int) ( ship->vy - spaceobj->ypos )) < 10 &&
                      abs( (int) ( ship->vz - spaceobj->zpos )) < 10 )
            {
                /* Evading the sun added by Darrik Vequir */

                ship->hx = 10 * ship->vx;
                ship->hy = 10 * ship->vy;
                ship->hz = 10 * ship->vz;
                ship->energy -= ship->currspeed/10;
                ship->goalspeed = ship->mod->realspeed;
                ship->accel = get_acceleration(ship);
                echo_to_room( AT_RED , get_room_index(ship->pilotseat), "Automatic Override: Evading to avoid collision with sun!\n" );
                if ( ship->shipclass == FIGHTER_SHIP || ( ship->shipclass == MIDSIZE_SHIP && ship->mod->manuever > 50 ) )
                    space_set_shipstate( ship, SHIP_BUSY_3 );
                else if ( ship->shipclass == MIDSIZE_SHIP || ( ship->shipclass == CAPITAL_SHIP && ship->mod->manuever > 50 ) )
                    space_set_shipstate( ship, SHIP_BUSY_2 );
                else
                    space_set_shipstate( ship, SHIP_BUSY );
                break;
            }
            /*
            echo_to_cockpit( AT_BLOOD+AT_BLINK, ship, "You fly directly into the sun.");
            SPRINTF( buf , "%s flies directly into %s!", ship->name, ship->spaceobject->star2);
            echo_to_system( AT_ORANGE , ship , buf , NULL );
            destroy_ship(ship , NULL);
            continue;
            */

            if ( ship->currspeed > 0 )
            {
              if ( spaceobj->type >= SPACE_PLANET && spaceobj->name && strcmp(spaceobj->name,"") &&
                  abs( (int) ( ship->vx - spaceobj->xpos )) < 10 &&
                        abs( (int) ( ship->vy - spaceobj->ypos )) < 10 &&
                        abs( (int) ( ship->vz - spaceobj->zpos )) < 10 )
              {
                  buf = str_printf("You begin orbiting %s.", spaceobj->name); 
                  echo_to_cockpit( AT_YELLOW, ship, buf);
                  buf = str_printf("%s begins orbiting %s.", ship->name, spaceobj->name); 
                  echo_to_system( AT_ORANGE , ship , buf , NULL );
                  ship->inorbitof = spaceobj;
                  ship->currspeed = 0;
                  ship->goalspeed = 0;
              }            

            }
        }
    }
}

static void update_ships_hyperspace_and_sync(GameContext *game)
{
    SHIP_DATA *ship;
    SHIP_DATA *target = NULL;
    SPACE_DATA *spaceobj;
    std::string buf;

    for ( ship = first_ship; ship; ship = ship->next )  // This second round is necessary as if we did it in the first loop some ships may be 
                                                        // updated, while others are still pre-tick.  A second loop makes sure all ships have had 
                                                        // their positions updated that tick. DV 4-14-26
    {
        target = NULL;
        if (ship->shipstate == SHIP_HYPERSPACE)
        {
            float tx, ty, tz;
            float dist, origdist;

            ship->hyperdistance -= ship->mod->hyperspeed;

            dist = (float) ship->hyperdistance;
            origdist = (float) ship->orighyperdistance;

            if ( origdist == 0 ) // Not sure why origdist would ever be 0, but just in case, set it to 1 to avoid divide by zero errors.  DV 4-19-26
              origdist = 1;

            if ( dist == 0)
              dist = -1;

            tx = ( ship->vx - ship->jx );
            ty = ( ship->vy - ship->jy );
            tz = ( ship->vz - ship->jz );
            ship->cx = ship->vx - (tx*(dist/origdist));
            ship->cy = ship->vy - (ty*(dist/origdist));
            ship->cz = ship->vz - (tz*(dist/origdist));

            ship->count++;

            for( spaceobj = first_spaceobject; spaceobj; spaceobj = spaceobj->next )
            {
              if( space_in_range_h( ship, spaceobj ) )
              {
                  int damage;
                  echo_to_room( AT_YELLOW, get_room_index(ship->pilotseat), "Hyperjump complete.");
                  echo_to_ship( AT_YELLOW, ship, "The ship slams to a halt as it comes out of hyperspace.");
                  // 2/18/04 - Johnson - Modified call to reflect origin of object entering the system
                  //SPRINTF( buf ,"%s enters the starsystem at %.0f %.0f %.0f" , ship->name, ship->vx, ship->vy, ship->vz );
                  buf = str_printf("%s enters the starsystem from hyperspace at %.0f %.0f %.0f" , ship->name, ship->vx, ship->vy, ship->vz );
                  damage = 15* number_range( 1, 4 );
                  ship->hull -= damage;
                  echo_to_ship( AT_YELLOW, ship, "The hull cracks from the pressure.");
                  ship->vx = ship->cx;
                  ship->vy = ship->cy;
                  ship->vz = ship->cz;
                  ship_to_spaceobject( ship, ship->currjump );
                  ship->currjump = NULL;
                  echo_to_system( AT_YELLOW, ship, buf , NULL );
                  space_set_shipstate( ship, SHIP_READY );
                  STRFREE( ship->home );
                  ship->home = STRALLOC( ship->spaceobject->name );
                  break;
              }
            }
            if ( ship->shipstate == SHIP_HYPERSPACE )
            {
                for( target = first_ship; target; target= target->next )
                {
                    if (target && ship && target != ship )
                    {
                        if ( target->spaceobject && ship->spaceobject && 
                            target->shipstate != SHIP_LANDED && 
                            target->mod && target->mod->gravproj && target->mod->gravitypower &&
                            space_distance_ship_less_than( ship, target, 10*target->mod->gravproj ))
                        break;
                    }
                }
            }   
            if( ship->shipstate == SHIP_HYPERSPACE && target )
            {
                echo_to_room( AT_YELLOW, get_room_index(ship->pilotseat), "Hyperjump complete.");
                echo_to_ship( AT_YELLOW, ship, "The ship slams to a halt as it comes out of hyperspace.  An artificial gravity well surrounds you!");
                // 2/18/04 - Johnson - Modified call to reflect origin of object entering the system
                //SPRINTF( buf ,"%s enters the starsystem at %.0f %.0f %.0f" , ship->name, ship->vx, ship->vy, ship->vz );
                buf = str_printf("%s slams into a gravity well at %.0f %.0f %.0f" , ship->name, ship->vx, ship->vy, ship->vz );
                ship->vx = ship->cx;
                ship->vy = ship->cy;
                ship->vz = ship->cz;
                ship_to_spaceobject( ship, ship->currjump );
                ship->currjump = NULL;
                echo_to_system( AT_YELLOW, ship, buf , NULL );
                space_set_shipstate( ship, SHIP_READY );
                STRFREE( ship->home );
                ship->home = STRALLOC( ship->spaceobject->name );
            }


            if (ship->shipstate == SHIP_HYPERSPACE && ship->hyperdistance <= 0 && !ship->tracking)
            {
                ship->count = 0;
                ship_to_spaceobject (ship, ship->currjump);
            	
                if (ship->spaceobject == NULL)
                {
                    echo_to_cockpit( AT_RED, ship, "Ship lost in Hyperspace. Make new calculations.");
                }
                else
                {
                    echo_to_room( AT_YELLOW, get_room_index(ship->pilotseat), "Hyperjump complete.");
                    echo_to_ship( AT_YELLOW, ship, "The ship lurches slightly as it comes out of hyperspace.");
					          // 2/18/04 - Johnson - Modified call to reflect origin of object entering the system
            	      //SPRINTF( buf ,"%s enters the starsystem at %.0f %.0f %.0f" , ship->name, ship->vx, ship->vy, ship->vz );
					          buf = str_printf("%s enters the starsystem from hyperspace at %.0f %.0f %.0f" , ship->name, ship->vx, ship->vy, ship->vz );
                    ship->cx = ship->vx;
                    ship->cy = ship->vy;
                    ship->cz = ship->vz;
                    ship_to_spaceobject( ship, ship->currjump );
                    ship->currjump = NULL;
                    echo_to_system( AT_YELLOW, ship, buf , NULL );
                    space_set_shipstate( ship, SHIP_READY );
                    STRFREE( ship->home );
                    ship->home = STRALLOC( ship->spaceobject->name );
            	  }
            }
            else if ( ( ship->count >= (ship->tcount ? ship->tcount : 10 ) ) && ship->shipstate == SHIP_HYPERSPACE && ship->tracking == TRUE)
            {
                ship_to_spaceobject (ship, ship->currjump);
                if (ship->spaceobject == NULL)
                {
                    echo_to_cockpit( AT_RED, ship, "Ship lost in Hyperspace. Make new calculations.");
                }
                else
                {

                    echo_to_room( AT_YELLOW, get_room_index(ship->pilotseat), "Hyperjump complete.");
                    echo_to_ship( AT_YELLOW, ship, "The ship lurches slightly as it comes out of hyperspace.");
                    // 2/18/04 - Johnson - Modified call to reflect origin of object entering the system
                    //SPRINTF( buf ,"%s enters the starsystem at %.0f %.0f %.0f" , ship->name, ship->vx, ship->vy, ship->vz );
                    ship->vx = ship->cx;
                    ship->vy = ship->cy;
                    ship->vz = ship->cz;
                    ship_to_spaceobject( ship, ship->currjump );
                    ship->currjump = NULL;
                    echo_to_system( AT_YELLOW, ship, str_printf("%s enters the starsystem from hyperspace at %.0f %.0f %.0f", ship->name, ship->vx, ship->vy, ship->vz) , NULL );
                    space_set_shipstate( ship, SHIP_READY );
                    STRFREE( ship->home );
                    ship->home = STRALLOC( ship->spaceobject->name );
                    //speed = ship->mod->hyperspeed;

                    ship->jx = ship->vx + ship->tx;
                    ship->jy = ship->vy + ship->ty;
                    ship->jz = ship->vz + ship->tz;

                    for( spaceobj = first_spaceobject; spaceobj; spaceobj = spaceobj->next )
                    {
                      if( space_in_range( ship, spaceobj ) )
                      {
                        ship->currjump = spaceobj;
                        break;
                      }
                    }
                    if( !spaceobj )
                      ship->currjump = ship->spaceobject;

                    ship->hyperdistance  = abs( (int) ( ship->vx - ship->jx )) ;
                        ship->hyperdistance += abs( (int) ( ship->vy - ship->jy )) ;
                    ship->hyperdistance += abs( (int) ( ship->vz - ship->jz )) ;
                    ship->hyperdistance /= 50;

                    ship->orighyperdistance = ship->hyperdistance;
                    
                    ship->count = 0;
                    if ( ship->ch )
                        do_radar( ship->ch, "" );
          	    }
            }
            
            else if (ship->count >= 10 && ship->shipstate == SHIP_HYPERSPACE)
            {
               ship->count = 0;
               echo_to_room_dnr( AT_YELLOW , get_room_index(ship->pilotseat), "Remaining jump distance: " );
               echo_to_room( AT_WHITE , get_room_index(ship->pilotseat),  str_printf("%d" , ship->hyperdistance) );
               
            }
            if( ship->shipstate == SHIP_HYPERSPACE )
            {
              if( ship->count%2 && ship->hyperdistance < 10*ship->mod->hyperspeed && ship->hyperdistance > 0 )
              {
                echo_to_ship( AT_RED , ship,  str_printf("An alarm sounds. Your hyperjump is ending: %d" , ship->hyperdistance) );
               
              }
            }
        }
        space_enforce_ship_invariants( ship );
        if( ship->docked )
        {
            space_ship_force_docked_match( ship, ship->docked );
        }  
     
        if( ship->vx > MAX_COORD)
          ship->vx = -MAX_COORD_S;
        if( ship->vy > MAX_COORD)
          ship->vy = -MAX_COORD_S;
        if( ship->vz > MAX_COORD)
          ship->vz = -MAX_COORD_S;
        if( ship->vx < -MAX_COORD)
          ship->vx = MAX_COORD_S;
        if( ship->vy < -MAX_COORD)
          ship->vy = MAX_COORD_S;
        if( ship->vz < -MAX_COORD)
          ship->vz = MAX_COORD_S;
    }
}

void move_ships( GameContext *game )
{
    move_spaceobjects(game);
    move_missiles(game);
    update_ships_realspace(game);
    update_ships_hyperspace_and_sync(game);
}   

static bool recharge_update_bay_cycle()
{
   static int baycount = 0;
   baycount++;

   if ( baycount >= 60 )
   {
     baycount = 0;
     return TRUE;
   }
   return FALSE;
}

static void recharge_ship_noncombat( GameContext *game, SHIP_DATA *ship, bool closeem)
{
    TURRET_DATA *turret;

    if (!ship->game)
        ship->game = game;
    if ( ship->shipclass == 3 )
        if ( closeem && ship->guard )
        ship->bayopen = FALSE;
        
    if (ship->statet0 > 0)
    {
        ship->energy -= ship->statet0;
        ship->statet0 = 0;
    }
    if (ship->statei0 > 0)
    {
        ship->energy -= 10*ship->statei0;
        ship->statei0 = 0;
    }

    if (ship->first_turret)
        for( turret = ship->first_turret; turret; turret = turret->next )
        if (turret->state > 0)
        {
            ship->energy -= turret->state;
            turret->state = 0;
        }

    space_validate_ship_links( NULL, ship, false );
    if( ship->docked && ship->docked->shipclass == SHIP_PLATFORM )
    {
        if( ship->mod->maxenergy - ship->energy > 500 )
        ship->energy += 500;
        else
        ship->energy = ship->mod->maxenergy;
    }

    if (ship->missilestate == MISSILE_RELOAD_2)
    {
        ship->missilestate = MISSILE_READY;
        if ( ship->missiles > 0 )
            echo_to_room( AT_YELLOW, get_room_index(ship->gunseat), "Missile launcher reloaded.");
    }
    
    if (ship->missilestate == MISSILE_RELOAD )
    {
        ship->missilestate = MISSILE_RELOAD_2;
    }
    
    if (ship->missilestate == MISSILE_FIRED )
        ship->missilestate = MISSILE_RELOAD;    
}

static void recharge_ship_autofly_combat(SHIP_DATA *ship)
{
    std::string buf;
    int origchance = 100;
    if ( autofly(ship) )
    {
        if ( ship->spaceobject && ship->shipclass != SHIP_DEBRIS )
        {
            if (ship->target0 && ship->statet0 != LASER_DAMAGED )
            {
                int chance = 75;
                SHIP_DATA * target = ship->target0;
                int shots, guns, laserguns, ionguns;
                int whichguns = 0;
                int laserhits = 0, lasermisses = 0;
                int ionhits = 0, ionmisses = 0;
                int laserdamage = 0, iondamage = 0;
                int damage = 0;
                
                if ( ship->mod->lasers && ship->mod->ions && ship->mod->lasers < 7 && ship->mod->ions < 7 )
                {
                    whichguns = 2;
                    guns = ship->mod->lasers + ship->mod->ions;
                    laserguns = ship->mod->lasers;
                    ionguns = ship->mod->ions;
                }
                else if ( ship->target0->shield > 0 && ship->mod->ions )
                {
                    whichguns = 1;
                    guns = ship->mod->ions;
                    laserguns = 0;
                    ionguns = ship->mod->ions;
                }
                else
                {
                    guns = ship->mod->lasers;
                    laserguns = ship->mod->lasers;
                    ionguns = 0;
                }
                
                for ( shots=0 ; shots < guns; shots++ )
                {
                    if ( !ship->target0 )
                        break;
                    if (ship->shipstate != SHIP_HYPERSPACE && ship->energy > 25 
                    && ship_target_in_combat_range( ship, target, 1000 ) )
                    {
                        if ( ship->shipclass > 1 || is_facing ( ship , target ) )
                        {
                            chance = compute_autofly_laser_ion_hit_chance( chance, origchance, ship, target);

                            if ( number_percent( ) > chance )
                            {
                                whichguns == 0 ? lasermisses++ : ( whichguns == 1 ? ionmisses++ : ( shots < ship->mod->lasers ? lasermisses++ : ionmisses++ ) );
                            } 
                            else
                            {
                                if( whichguns == 0 )
                                {
                                    laserhits++;
                                    if ( ship->shipclass == SHIP_PLATFORM ) 
                                    {      
                                        damage = number_range( SHIPDAM_MINPLATFORM , SHIPDAM_MAXPLATFORM );
                                    }
                                    else if( ship->shipclass == CAPITAL_SHIP && target->shipclass != CAPITAL_SHIP )
                                        damage = number_range( SHIPDAM_MINCAP, SHIPDAM_MAXCAP );
                                    else 
                                        damage = number_range( SHIPDAM_MINDEF , SHIPDAM_MAXDEF );
                    
                                    laserdamage += damage;
                    
                                }
                                else if( whichguns == 1 )
                                {
                                    ionhits++;
                                    if ( ship->shipclass == SHIP_PLATFORM ) 
                                    {      
                                        damage = number_range( SHIPDAM_MINPLATFORM , SHIPDAM_MAXPLATFORM );
                                    }
                                    else if( ship->shipclass == CAPITAL_SHIP && target->shipclass != CAPITAL_SHIP )
                                        damage = number_range( SHIPDAM_MINCAP, SHIPDAM_MAXCAP );
                                    else 
                                        damage = number_range( SHIPDAM_MINDEF , SHIPDAM_MAXDEF );
                    
                                    iondamage += damage;
                    
                    
                                } 
                                else if( whichguns == 2 )
                                {
                                    if( shots < ship->mod->lasers )
                                    {
                                        laserhits++;
                                        if ( ship->shipclass == SHIP_PLATFORM ) 
                                        {      
                                            damage = number_range( SHIPDAM_MINPLATFORM , SHIPDAM_MAXPLATFORM );
                                        }
                                        else if( ship->shipclass == CAPITAL_SHIP && target->shipclass != CAPITAL_SHIP )
                                            damage = number_range( SHIPDAM_MINCAP, SHIPDAM_MAXCAP );
                                        else 
                                            damage = number_range( SHIPDAM_MINDEF , SHIPDAM_MAXDEF );
                    
                                        laserdamage += damage;
                                    }
                                        else
                                    {
                                        ionhits++;
                                        if ( ship->shipclass == SHIP_PLATFORM ) 
                                        {      
                                            damage = number_range( SHIPDAM_MINPLATFORM , SHIPDAM_MAXPLATFORM );
                                        }
                                        else if( ship->shipclass == CAPITAL_SHIP && target->shipclass != CAPITAL_SHIP )
                                            damage = number_range( SHIPDAM_MINCAP, SHIPDAM_MAXCAP );
                                        else 
                                            damage = number_range( SHIPDAM_MINDEF , SHIPDAM_MAXDEF );
                    
                                        iondamage += damage;
                                    }
                                }
                            }
                    
                        }
                    }
                }

                if ( laserhits )
                {
                    if ( lasermisses )
                    {
                        buf = str_printf("%s fires %d lasers at you, hitting you %d times.\n", 
                                ship->name, laserguns, laserhits );
                                    echo_to_cockpit( AT_BLOOD , target , buf );           
                        buf = str_printf("%s fires %d lasers at %s, hitting %d times.\n", 
                                ship->name, laserguns, target->name, laserhits );
                                    echo_to_system( AT_ORANGE , target , buf , NULL );

                    }
                    else
                    {
                        buf = str_printf("%s fires %d lasers at you, hitting you %d times.\n", 
                                ship->name, laserguns, laserhits );
                                    echo_to_cockpit( AT_BLOOD , target , buf );           
                        buf = str_printf("%s fires %d lasers at %s, hitting %d times.\n", 
                                ship->name, laserguns, target->name, laserhits );
                                    echo_to_system( AT_ORANGE , target , buf , NULL );
                    }
                    
                    damage_ship ( target, ship, laserdamage, laserdamage+1 );
                }
                else if ( lasermisses )
                {
                    buf = str_printf("%s fires its lasers at you, missing you %d times.\n", 
                            ship->name, lasermisses );
                    echo_to_cockpit( AT_BLOOD , target , buf );           
                    buf = str_printf("%s fires its lasers at %s, missing %d times.\n", 
                            ship->name, target->name, lasermisses );
                    echo_to_system( AT_ORANGE , target , buf , NULL );
                }
                    
                if ( ionhits ) 
                {
                    if ( ionmisses )
                    {
                        buf = str_printf("%s fires %d ion cannons at you, hitting you %d times.\n", 
                            ship->name, ionguns, ionhits );
                        echo_to_cockpit( AT_BLOOD , target , buf );           
                        buf = str_printf("%s fires %d ion cannons at %s, hitting %d times.\n", 
                            ship->name, ionguns, target->name, ionhits );
                        echo_to_system( AT_ORANGE , target , buf , NULL );
                    }
                    else
                    {
                        buf = str_printf("%s fires %d ion cannons at you, hitting you %d times.\n", 
                            ship->name, ionguns, ionhits );
                        echo_to_cockpit( AT_BLOOD , target , buf );           
                        buf = str_printf("%s fires %d ion cannons at %s, hitting %d times.\n", 
                            ship->name, ionguns, target->name, ionhits );
                        echo_to_system( AT_ORANGE , target , buf , NULL );
                    }
                    
                    damage_ship ( target, ship, -1*iondamage, -1*(iondamage-1) );
                    
                }		
                else if ( ionmisses )
                {
                    buf = str_printf("%s fires its ion cannons at you, missing you %d times.\n", 
                            ship->name, ionmisses );
                    echo_to_cockpit( AT_BLOOD , target , buf );           
                    buf = str_printf("%s fires its ions cannons at %s, missing %d times.\n", 
                            ship->name, target->name, ionmisses );
                    echo_to_system( AT_ORANGE , target , buf , NULL );
                    
                }
    

            }
        }
    }    
}
void recharge_ships( GameContext *game )
{
   SHIP_DATA *ship;
   bool closeem = recharge_update_bay_cycle();

   for ( ship = first_ship; ship; ship = ship->next )
   {                
        recharge_ship_noncombat(game, ship, closeem);
        recharge_ship_autofly_combat(ship);
   }
}

static void update_space_state_update_and_displays(GameContext *game)
{
    SHIP_DATA *ship;
    SHIP_DATA *target;
    std::string buf;
    int too_close, target_too_close;
    SPACE_DATA *spaceobj;
    TURRET_DATA *turret;
    int recharge;

    for ( ship = first_ship; ship; ship = ship->next )
    {
        if (!ship->game)
            ship->game = game;

        if ( ship->spaceobject && ship->energy > 0 && ship->shipstate == SHIP_DISABLED && ship->shipclass != SHIP_PLATFORM )
            ship->energy -= 100;
        else if (ship->energy > 0 && ( !ship->mod || !ship->mod->gravproj ) ) // Energy does not recharge when gravity wells are activated  - DV 8/7/02
            ship->energy += (5 + ship->shipclass * 5);
        else if (ship->mod && ship->mod->gravproj ) // Energy drains 1/100th of gravity power every space pulse - DV 8/7/02
            ship->energy -= ship->mod->gravproj/100;
        else
            ship->energy = 0;

        if ( ship->chaff_released > 0 )
            ship->chaff_released--;
        
        /* following was originally to fix ships that lost their pilot 
           in the middle of a maneuver and are stuck in a busy state 
           but now used for timed maneuvers such as turning */
    
        if( ship->shipstate == SHIP_READY && ship->tracking == TRUE )
	      {
            if( ship->count == 0 )
            {
                ship->count++;
            }
            else
            {
                if ( ship->ch )
                    do_hyperspace( ship->ch, "" );
                else
                    ship->tracking = FALSE;
                ship->count = 0;
            }
        }         
    
    	  if (ship->shipstate == SHIP_BUSY_3)
            {
                echo_to_room( AT_YELLOW, get_room_index(ship->pilotseat), "Manuever complete.");
                space_set_shipstate( ship, SHIP_READY );
            }
        if (ship->shipstate == SHIP_BUSY_2)
            space_set_shipstate( ship, SHIP_BUSY_3 );
        if (ship->shipstate == SHIP_BUSY)
            space_set_shipstate( ship, SHIP_BUSY_2 );
        
        if (ship->shipstate == SHIP_LAND_2)
        {
            if (!ship->dest || ship->dest[0] == '\0')
            {
                if (ship->shipstate != SHIP_DISABLED)
                    space_set_shipstate( ship, SHIP_READY );
            }
            else
                landship( ship , ship->dest );
        }
        if (ship->shipstate == SHIP_LAND)
        {
            if (!ship->dest || ship->dest[0] == '\0')
            {
                if (ship->shipstate != SHIP_DISABLED)
                    space_set_shipstate( ship, SHIP_READY );
            }
            else
            {
                approachland( ship, ship->dest );
                if (ship->shipstate == SHIP_LAND)
                    space_set_shipstate( ship, SHIP_LAND_2 );
            }
        }
        
        if (ship->shipstate == SHIP_LAUNCH_2)
            launchship( ship );
        if (ship->shipstate == SHIP_LAUNCH)
            space_set_shipstate( ship, SHIP_LAUNCH_2 );

        if (ship->docking == SHIP_DOCK_2)
            dockship( ship->ch , ship );
        if (ship->docking == SHIP_DOCK)
            space_set_docking_state( ship, SHIP_DOCK_2 );
        

        ship->shield = UMAX( 0 , ship->shield-1-ship->shipclass);

        if ( ship->mod ) // Unlikely guard in case mod is NULL - DV 4-20-26
        {
            if (ship->autorecharge && ship->mod->maxshield > ship->shield && ship->energy > 100)
            {
                recharge  = UMIN( ship->mod->maxshield-ship->shield, 10 + ship->shipclass*10 );           
                recharge  = UMIN( recharge , ship->energy/2 -100 );
                recharge  = UMAX( 1, recharge );
                ship->shield += recharge;
                ship->energy -= recharge;        
            }
            
            if ( autofly(ship) && ( ship->energy >= ((25+ship->shipclass*25)*(2+ship->shipclass) ) )
            && ((ship->mod->maxshield - ship->shield) >= ( 25+ship->shipclass*25 ) ) )
            {
                recharge  = 25+ship->shipclass*25;
                recharge  = UMIN(  ship->mod->maxshield-ship->shield , recharge );
                ship->shield += recharge;
                ship->energy -= ( recharge*2 + recharge * ship->shipclass );        
            }
        }
        
        if (ship->shield > 0)
        {
            if (ship->energy < 200)
            {
                ship->shield = 0;
                echo_to_cockpit( AT_RED, ship,"The ships shields fizzle and die.");
                ship->autorecharge = FALSE;
            }
        }        
        
        if ( ship->spaceobject && ship->currspeed > 0 )
        {
            echo_to_room_dnr( AT_BLUE , get_room_index(ship->pilotseat),  "Speed: " );
            echo_to_room_dnr( AT_LBLUE , get_room_index(ship->pilotseat),  str_printf("%d", ship->currspeed) );
            echo_to_room_dnr( AT_BLUE , get_room_index(ship->pilotseat),  "  Coords: " );
            echo_to_room( AT_LBLUE , get_room_index(ship->pilotseat),  str_printf("%.0f %.0f %.0f", ship->vx , ship->vy, ship->vz) );
            if ( ship->pilotseat != ship->coseat )
            {

                echo_to_room_dnr( AT_BLUE , get_room_index(ship->coseat),  "Speed: " );
                echo_to_room_dnr( AT_LBLUE , get_room_index(ship->coseat),  str_printf("%d", ship->currspeed) );
                echo_to_room_dnr( AT_BLUE , get_room_index(ship->coseat),  "  Coords: " );
                echo_to_room( AT_LBLUE , get_room_index(ship->coseat),  str_printf("%.0f %.0f %.0f", ship->vx , ship->vy, ship->vz) );
            }
        } 

        if ( ship->spaceobject )
        {
            too_close = ship->currspeed + 50;

            for( spaceobj = first_spaceobject; spaceobj; spaceobj = spaceobj->next )
            {
                if ( spaceobj->name &&  strcmp(spaceobj->name,"") && 
                      abs( (int) ( ship->vx - spaceobj->xpos )) < too_close && 
                      abs( (int) ( ship->vy - spaceobj->ypos )) < too_close &&
                      abs( (int) ( ship->vz - spaceobj->zpos )) < too_close )
                {
                      echo_to_room( AT_RED , get_room_index(ship->pilotseat),  str_printf("Proximity alert: %s  %.0f %.0f %.0f", spaceobj->name,
                                spaceobj->xpos, spaceobj->ypos, spaceobj->zpos) );
                }
            }
            for ( target = first_ship; target; target = target->next )
            { 
                if( (target->docked && target->docked == ship) || (ship->docked &&  ship->docked == target ) )
                    continue;
                if ( ship->docked && target->docked && 
                      target->docked == ship->docked )
                    continue;
                
                target_too_close = too_close+target->currspeed;
                if( target->spaceobject )
                  if ( target != ship &&
                    space_distance_ship_less_than( ship, target, target_too_close ) &&
                    ship->docked != target && target->docked != ship )
                  {
                      echo_to_room( AT_RED , get_room_index(ship->pilotseat),  str_printf("Proximity alert: %s  %.0f %.0f %.0f", target->name, target->vx - ship->vx, target->vy - ship->vy, target->vz - ship->vz) );    
                  }                
            }
            too_close = ship->currspeed + 100;
        }

        if (ship->target0 && ship->shipclass <= SHIP_PLATFORM)
        {
            echo_to_room_dnr( AT_BLUE , get_room_index(ship->gunseat), "Target: " );
            echo_to_room( AT_LBLUE , get_room_index(ship->gunseat),  str_printf("%s   %.0f %.0f %.0f", ship->target0->name,
                      ship->target0->vx , ship->target0->vy, ship->target0->vz) );
            if (!ship_in_range( ship, ship->target0 ) )
            {
                echo_to_room( AT_LBLUE , get_room_index(ship->gunseat),  "Your target seems to have left.");
                ship->target0 = NULL;
            }
        }
        
        if ( ship->first_turret )
        {
          for( turret = ship->first_turret; turret; turret = turret->next )
            if( turret->target && ship->shipclass <= SHIP_PLATFORM )
            {
              echo_to_room_dnr( AT_BLUE , get_room_index(turret->roomvnum), "Target: " );
              echo_to_room( AT_LBLUE , get_room_index(turret->roomvnum),  str_printf("%s   %.0f %.0f %.0f", turret->target->name,
                        turret->target->vx , turret->target->vy, turret->target->vz) );
              if (!ship_in_range( ship, turret->target ) )
              {
                turret->target = NULL;
                echo_to_room( AT_LBLUE , get_room_index(turret->roomvnum),  "Your target seems to have left.");
              }
            }
        }   	
        
        if (ship->energy < 100 && ship->spaceobject )
        {
            echo_to_cockpit( AT_RED , ship,  "Warning: Ship fuel low." );
        }
        
        if ( ship->mod )        
            ship->energy = URANGE( 0 , ship->energy, ship->mod->maxenergy );
    }     
}

static void update_autofly_set_stop( SHIP_DATA *ship )
{
    ship->goalspeed = 0;
    ship->accel = get_acceleration( ship );
}

static void update_autofly_target_departure( SHIP_DATA *ship )
{
    if ( ship->target0 && autofly(ship) )
        if ( !ship_in_range( ship->target0, ship ) )
        {
            echo_to_room( AT_BLUE , get_room_index(ship->pilotseat), "Target left, returning to NORMAL condition.\n" );
            update_autofly_set_stop( ship );
            ship->target0 = NULL;
        }
}

static void update_autotrack_set_busy_state( SHIP_DATA *ship )
{
    if ( ship->shipclass == FIGHTER_SHIP || ( ship->shipclass == MIDSIZE_SHIP && ship->mod->manuever > 50 ) )
        space_set_shipstate( ship, SHIP_BUSY_3 );
    else if ( ship->shipclass == MIDSIZE_SHIP || ( ship->shipclass == CAPITAL_SHIP && ship->mod->manuever > 50 ) )
        space_set_shipstate( ship, SHIP_BUSY_2 );
    else
        space_set_shipstate( ship, SHIP_BUSY );
}

static void update_autotrack_apply_course_change( SHIP_DATA *ship, SHIP_DATA *target, bool evade, sh_int msgcolor, const char *msg )
{
    if ( evade )
    {
        ship->hx = 0 - ( target->vx - ship->vx );
        ship->hy = 0 - ( target->vy - ship->vy );
        ship->hz = 0 - ( target->vz - ship->vz );
    }
    else
    {
        ship->hx = target->vx - ship->vx;
        ship->hy = target->vy - ship->vy;
        ship->hz = target->vz - ship->vz;
    }

    ship->energy -= ship->currspeed / 10;
    echo_to_room( msgcolor , get_room_index(ship->pilotseat), msg );
    update_autotrack_set_busy_state( ship );
}

static void update_autotrack_course( SHIP_DATA *ship )
{
    SHIP_DATA *target;
    int too_close, target_too_close;

    if ( ship->autotrack && ship->docking == SHIP_READY && ship->target0 && ship->shipclass < 3 )
    {
        bool not_docked;

        target = ship->target0;
        too_close = ship->currspeed + 10;
        target_too_close = too_close + target->currspeed;
        not_docked = space_ship_is_not_docked( ship );

        if ( target != ship
          && space_ship_is_ready( ship )
          && not_docked
          && space_distance_ship_less_than( ship, target, target_too_close ) )
        {
            update_autotrack_apply_course_change(
                ship, target, true, AT_RED, "Autotrack: Evading to avoid collision!\n" );
        }
        else if ( !is_facing(ship, target)
               && not_docked )
        {
            update_autotrack_apply_course_change(
                ship, target, false, AT_BLUE, "Autotracking target ... setting new course.\n" );
        }
    }
}

static void update_capital_ship_resupply( SHIP_DATA *ship )
{
    if ( ( ship->shipclass == CAPITAL_SHIP || ship->shipclass == SHIP_PLATFORM )
    && ship->target0 == NULL )
    {
        int ammo = ship->missiles + ship->torpedos * 2 + ship->rockets * 3;

        if ( ammo < ship->mod->launchers * 8 )
            ship->missiles++;
        if ( ammo < ship->mod->launchers * 8 * 2 )
            ship->torpedos++;
        if ( ammo < ship->mod->launchers * 8 * 3 )
            ship->rockets++;
        if ( ship->chaff < ship->mod->defenselaunchers * 6 )
            ship->chaff++;
    }
}

static void update_autofly_repair_states( SHIP_DATA *ship )
{
    if( ship->missilestate ==  MISSILE_DAMAGED )
        ship->missilestate =  MISSILE_READY;
    if( ship->statet0 ==  LASER_DAMAGED )
        ship->statet0 =  LASER_READY;
    if( ship->statei0 ==  LASER_DAMAGED )
        ship->statei0 =  LASER_READY;
    if( ship->shipstate ==  SHIP_DISABLED )
        space_set_shipstate( ship, SHIP_READY );
}

static void update_autofly_no_target_slowdown( SHIP_DATA *ship )
{
    if( ship->currspeed )
        update_autofly_set_stop( ship );
}

static void update_autofly_relocation( SHIP_DATA *ship )
{
    if ( number_range(1, 25) == 25 )
    {
        ship_to_spaceobject(ship, spaceobject_from_name(ship->game, ship->home) );
        if( ship->spaceobject )
        {
            ship->vx = ship->spaceobject->xpos + number_range( -5000 , 5000 );
            ship->vy = ship->spaceobject->ypos + number_range( -5000 , 5000 );
            ship->vz = ship->spaceobject->zpos + number_range( -5000 , 5000 );
            ship->hx = 1;
            ship->hy = 1;
            ship->hz = 1;
        }
    }
}

static SHIP_DATA *find_autofly_assist_ship( SHIP_DATA *ship )
{
    SHIP_DATA *target;

    for ( target = first_ship; target; target = target->next)
    {
        if( ship_in_range( ship, target ) )
          if ( autofly(target) && target->docked == NULL && target->shipstate != SHIP_DOCKED )
            if ( !str_cmp ( target->owner , ship->owner ) && target != ship )
              if ( target->target0 == NULL && ship->target0 != target )
                return target;
    }

    return NULL;
}

static void update_autofly_apply_assist_target( SHIP_DATA *ship, SHIP_DATA *assist_ship )
{
    std::string buf;

    if ( !assist_ship )
        return;

    assist_ship->target0 = ship->target0;
    buf = str_printf("You are being targeted by %s." , assist_ship->name);
    echo_to_cockpit( AT_BLOOD , assist_ship->target0 , buf );
}

static void update_autofly_auto_assist( SHIP_DATA *ship )
{
    SHIP_DATA *assist_ship;

    if (!ship || !ship->target0 )
        return;

    if ( !ship->target0->target0 && autofly(ship->target0))
      ship->target0->target0 = ship;

    assist_ship = find_autofly_assist_ship( ship );
    update_autofly_apply_assist_target( ship, assist_ship );
}

static bool update_autofly_can_try_projectile_launch( SHIP_DATA *ship, SHIP_DATA *target )
{
    if ( !ship || !target )
        return false;
    if ( ship->shipstate == SHIP_HYPERSPACE )
        return false;
    if ( ship->energy <= 25 )
        return false;
    if ( ship->missilestate != MISSILE_READY )
        return false;
    if ( !ship_target_in_combat_range( ship, target, 1200 ) )
        return false;
    if ( !( ship->shipclass > 1 || is_facing( ship , target ) ) )
        return false;

    return true;
}

static void update_autofly_consume_projectile_inventory( SHIP_DATA *ship, int projectiles )
{
    if( projectiles == CONCUSSION_MISSILE ) ship->missiles--;
    if( projectiles == PROTON_TORPEDO ) ship->torpedos--;
    if( projectiles == HEAVY_ROCKET ) ship->rockets--;
}

static void update_autofly_execute_projectile_launch( SHIP_DATA *ship, SHIP_DATA *target, int projectiles )
{
    std::string buf;

    if ( !ship || !target )
        return;

    if ( projectiles != CONCUSSION_MISSILE
      && projectiles != PROTON_TORPEDO
      && projectiles != HEAVY_ROCKET )
        return;

    new_missile( ship , target , NULL , projectiles );
    update_autofly_consume_projectile_inventory( ship, projectiles );
    buf = str_printf("Incoming projectile from %s." , ship->name);
    echo_to_cockpit( AT_BLOOD , target , buf );
    buf = str_printf("%s fires a projectile towards %s." , ship->name, target->name );
    echo_to_system( AT_ORANGE , target , buf , NULL );

    if ( ship->shipclass == CAPITAL_SHIP || ship->shipclass == SHIP_PLATFORM )
        ship->missilestate = MISSILE_RELOAD_2;
    else
        ship->missilestate = MISSILE_FIRED;
}

static void update_autofly_try_projectile_launch( SHIP_DATA *ship, SHIP_DATA *target )
{
    int chance = 50;
    int projectiles = -1;

    if ( !update_autofly_can_try_projectile_launch( ship, target ) )
        return;

    chance = compute_autopilot_projectile_launch_chance( chance, ship, target);
    projectiles = select_autopilot_projectile_type( ship, target );

    if ( number_percent( ) <= chance && projectiles != -1 )
        update_autofly_execute_projectile_launch( ship, target, projectiles );
}

static void update_autofly_engagement_state( SHIP_DATA *ship )
{
    ship->autotrack = TRUE;
    if( ship->shipclass != SHIP_PLATFORM && !ship->guard
        && ship->docked == NULL && ship->shipstate != SHIP_DOCKED )
    {
        ship->goalspeed = ship->mod->realspeed;
        ship->accel = get_acceleration( ship );
    }
    if ( ship->energy >200  )
        ship->autorecharge=TRUE;
}

static void update_autofly_target_combat( SHIP_DATA *ship )
{
    SHIP_DATA *target;

    update_autofly_auto_assist( ship );

    if ( ship->target0 != NULL )
    {
        target = ship->target0;
        update_autofly_engagement_state( ship );

        update_autofly_try_projectile_launch( ship, target );
    }
    update_autofly_repair_states( ship );
}

static void update_autofly_combat_and_spaceobject( SHIP_DATA *ship )
{
    if ( !autofly(ship) || ship->shipclass == SHIP_DEBRIS )
        return;

    if ( !ship->spaceobject )
    {
        update_autofly_relocation( ship );
        return;
    }

    check_hostile( ship );

    if ( ship->target0 )
        update_autofly_target_combat( ship );
    else
        update_autofly_no_target_slowdown( ship );
}

static void update_space_autopilot_and_resupply(GameContext *game)
{
    SHIP_DATA *ship;

    for ( ship = first_ship; ship; ship = ship->next )
    {
        update_autofly_target_departure( ship );

        update_autotrack_course( ship );

        update_autofly_combat_and_spaceobject( ship );

        update_capital_ship_resupply( ship );
      
        ship->lastsaved++;
        if ( ship->dirty || ship->lastsaved > 10 )
            save_ship( ship );
    }
}

void update_space( GameContext *game )
{
    update_space_state_update_and_displays(game);
    update_space_autopilot_and_resupply(game);
}



void write_spaceobject_list( GameContext *game )
{
    SPACE_DATA *tspaceobject;
    FILE *fpout;
    std::string filename;

    filename = str_printf("%s%s", SPACE_DIR, SPACE_LIST);
    fpout = fopen( filename.c_str(), "w" );
    if ( !fpout )
    {
         bug( "FATAL: cannot open space.lst for writing!\n", 0 );
         return;
    }
    for ( tspaceobject = first_spaceobject; tspaceobject; tspaceobject = tspaceobject->next )
      fprintf( fpout, "%s\n", tspaceobject->filename );
    fprintf( fpout, "$\n" );
    FCLOSE( fpout );
}


/*
 * Get pointer to space structure from spaceobject name.
 */
SPACE_DATA *spaceobject_from_name( GameContext *game, const std::string&name )
{
    SPACE_DATA *spaceobject;
    
    for ( spaceobject = first_spaceobject; spaceobject; spaceobject = spaceobject->next )
       if ( !str_cmp( name, spaceobject->name ) )
         return spaceobject;
    
    for ( spaceobject = first_spaceobject; spaceobject; spaceobject = spaceobject->next )
       if ( !str_prefix( name, spaceobject->name ) )
         return spaceobject;
    
    return NULL;
}

/*
 * Get pointer to space structure from the dock vnun.
 */
SPACE_DATA *spaceobject_from_vnum( GameContext *game, int vnum )
{
    SPACE_DATA *spaceobject;
    SHIP_DATA *ship;

    for ( spaceobject = first_spaceobject; spaceobject; spaceobject = spaceobject->next )
       if ( vnum == spaceobject->doca ||
            vnum == spaceobject->docb ||
            vnum == spaceobject->docc )
         return spaceobject;

    for ( ship = first_ship; ship; ship = ship->next )
       if ( vnum == ship->hanger )
       {
         if( !(ship->bayopen) )
           return NULL; 
         return ship->spaceobject;
        }

    return NULL;
}


/*
 * Save a spaceobject's data to its data file
 */
void save_spaceobject( SPACE_DATA *spaceobject )
{
    FILE *fp;
    std::string filename, buf;
    CARGO_DATA_LIST *cargolist;

    if ( !spaceobject )
    {
	bug( "save_spaceobject: null pointer!", 0 );
	return;
    }

    if ( !spaceobject->filename || spaceobject->filename[0] == '\0' )
    {
	buf = str_printf("save_spaceobject: %s has no filename", spaceobject->name );
	bug( buf.c_str(), 0 );
	return;
    }
 
    filename = str_printf("%s%s", SPACE_DIR, spaceobject->filename);
    
    FCLOSE( fpReserve );
    if ( ( fp = fopen( filename.c_str(), "w" ) ) == NULL )
    {
    	bug( "save_spaceobject: fopen", 0 );
    	perror( filename.c_str() );
    }
    else
    {
        fprintf( fp, "#SPACE\n" );
        fprintf( fp, "Name         %s~\n",	spaceobject->name	);
        fprintf( fp, "Filename     %s~\n",	spaceobject->filename	);
        fprintf( fp, "Type         %d\n",	spaceobject->type       );
        fprintf( fp, "Locationa      %s~\n",	spaceobject->locationa	);
        fprintf( fp, "Locationb      %s~\n",	spaceobject->locationb	);
        fprintf( fp, "Locationc      %s~\n",	spaceobject->locationc	);
        fprintf( fp, "Doca          %d\n",	spaceobject->doca	);
        fprintf( fp, "Docb          %d\n",      spaceobject->docb       );
        fprintf( fp, "Docc          %d\n",      spaceobject->docc       );        	
        fprintf( fp, "Seca          %d\n",	spaceobject->seca	);
        fprintf( fp, "Secb          %d\n",      spaceobject->secb       );
        fprintf( fp, "Secc          %d\n",      spaceobject->secc       );        	
        fprintf( fp, "Gravity     %d\n",       spaceobject->gravity    );
        fprintf( fp, "Xpos          %.0f\n",       spaceobject->xpos    );
        fprintf( fp, "Ypos          %.0f\n",       spaceobject->ypos    );	
        fprintf( fp, "Zpos          %.0f\n",       spaceobject->zpos    );	
        fprintf( fp, "HX            %.0f\n",       spaceobject->hx      );
        fprintf( fp, "HY            %.0f\n",       spaceobject->hy      );	
        fprintf( fp, "HZ            %.0f\n",       spaceobject->hz      );	
        fprintf( fp, "SP            %d\n",       spaceobject->speed   );	
        fprintf( fp, "Trainer       %d\n",       spaceobject->trainer    );	
        if ( spaceobject->first_cargo )
        {
            for( cargolist = spaceobject->first_cargo; cargolist; cargolist = cargolist->next )
            {
                fprintf( fp, "Cargo %d %d\n", 
                  cargolist->cargo->cargotype, cargolist->cargo->price );	
            }
        }
        fprintf( fp, "End\n\n"						);

        fprintf( fp, "#END\n"						);
    }
    FCLOSE( fp );
    fpReserve = fopen( NULL_FILE, "r" );
    return;
}


/*
 * Read in actual spaceobject data.
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

void fread_spaceobject( SPACE_DATA *spaceobject, FILE *fp )
{
    std::string buf;
    const char *word;
    bool fMatch;
    CARGO_DATA_LIST *cargolist;
    CARGO_DATA *cargo;

 
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

              case 'C':
            if ( !str_cmp( word, "Cargo" ) )
            {
                int x1,x2;
                char *ln = fread_line( fp );

                x1=x2=0;
                sscanf( ln, "%d %d", &x1, &x2 );

                CREATE( cargolist, CARGO_DATA_LIST, 1 );
              
                  LINK( cargolist, spaceobject->first_cargo, spaceobject->last_cargo, next, prev );
              
                  CREATE( cargo, CARGO_DATA, 1 );

                cargolist->cargo = cargo;
              
                cargolist->cargo->cargotype	= x1;
                cargolist->cargo->price	= x2;
                fMatch		= TRUE;
                break;
	          }

        case 'D':
             KEY( "Doca",  spaceobject->doca,          fread_number( fp ) );
             KEY( "Docb",  spaceobject->docb,          fread_number( fp ) );
             KEY( "Docc",  spaceobject->docc,          fread_number( fp ) );
             break;
                                

      case 'E':
          if ( !str_cmp( word, "End" ) )
          {
        if (!spaceobject->name)
          spaceobject->name		= STRALLOC( "" );
        if (!spaceobject->locationa)
          spaceobject->locationa            = STRALLOC( "" );  
        if (!spaceobject->locationb)
          spaceobject->locationb            = STRALLOC( "" );  
        if (!spaceobject->locationc)
          spaceobject->locationc            = STRALLOC( "" );  
        return;
          }
          break;
          
      case 'F':
          KEY( "Filename",	spaceobject->filename,		fread_string_nohash( fp ) );
          break;
            
            case 'G':
                KEY( "Gravity",  spaceobject->gravity,     fread_number( fp ) ); 
                break;  
            
            case 'H':
                KEY( "HX",  spaceobject->hx,     fread_number( fp ) ); 
                KEY( "HY",  spaceobject->hy,     fread_number( fp ) ); 
                KEY( "HZ",  spaceobject->hz,     fread_number( fp ) ); 
                break;  

            case 'L':
          KEY( "Locationa",	spaceobject->locationa,		fread_string( fp ) );
          KEY( "Locationb",	spaceobject->locationb,		fread_string( fp ) );
          KEY( "Locationc",	spaceobject->locationc,		fread_string( fp ) );
          break;
      
      case 'N':
          KEY( "Name",	spaceobject->name,		fread_string( fp ) );
          break;
    /*        
            case 'P':
                break;

                KEY( "Star1",	spaceobject->star1,	fread_string( fp ) );
          KEY( "Star2",	spaceobject->star2,	fread_string( fp ) );
          KEY( "S1x",  spaceobject->s1x,          fread_number( fp ) ); 
                KEY( "S1y",  spaceobject->s1y,          fread_number( fp ) ); 
                KEY( "S1z",  spaceobject->s1z,          fread_number( fp ) ); 
                KEY( "S2x",  spaceobject->s2x,          fread_number( fp ) ); 
                KEY( "S2y",  spaceobject->s2y,          fread_number( fp ) );
                KEY( "S2z",  spaceobject->s2z,          fread_number( fp ) );
    */
            case 'S':
                KEY( "Seca", spaceobject->seca, 		fread_number( fp ) );
                KEY( "Secb", spaceobject->secb, 		fread_number( fp ) );
                KEY( "Secc", spaceobject->secc, 		fread_number( fp ) );
                KEY( "SP", spaceobject->speed, 		fread_number( fp ) );
            case 'T':
                KEY( "Trainer",  spaceobject->trainer,     fread_number( fp ) ); 
                KEY( "Type",  spaceobject->type,	       fread_number( fp ) ); 
                
            case 'X':
                KEY( "Xpos",  spaceobject->xpos,     fread_number( fp ) ); 
            
            case 'Y':
                KEY( "Ypos",  spaceobject->ypos,     fread_number( fp ) ); 

            case 'Z':
                KEY( "Zpos",  spaceobject->zpos,     fread_number( fp ) ); 
                    
        }

        if ( !fMatch )
        {
            buf = str_printf("Fread_spaceobject: no match: %s", word);
            bug( buf.c_str(), 0 );
        }
    }
}

/*
 * Load a spaceobject file
 */

bool load_spaceobject( GameContext *game, const std::string& spaceobjectfile )
{
    std::string filename;
    SPACE_DATA *spaceobject;
    FILE *fp;
    bool found;

    CREATE( spaceobject, SPACE_DATA, 1 );
    spaceobject->game = game;

    found = FALSE;
    filename = str_printf("%s%s", SPACE_DIR, spaceobjectfile.c_str() );

    if ( ( fp = fopen( filename.c_str(), "r" ) ) != NULL )
    {

	found = TRUE;
        LINK( spaceobject, first_spaceobject, last_spaceobject, next, prev );
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
		bug( "Load_spaceobject_file: # not found.", 0 );
		break;
	    }

	    word = fread_word( fp );
	    if ( !str_cmp( word, "SPACE"	) )
	    {
	    	fread_spaceobject( spaceobject, fp );
	    	break;
	    }
	    else
	    if ( !str_cmp( word, "END"	) )
	        break;
	    else
	    {
		std::string buf;

		buf = str_printf("Load_spaceobject_file: bad section: %s.", word);
		bug( buf.c_str(), 0 );
		break;
	    }
	}
	FCLOSE( fp );
    }

    if ( !(found) )
      DISPOSE( spaceobject );

    return found;
}

/*
 * Load in all the spaceobject files.
 */
void load_space( GameContext *game )
{
    FILE *fpList;
    const char *filename;
    std::string spaceobjectlist;
    std::string buf;
    
    
    first_spaceobject	= NULL;
    last_spaceobject	= NULL;

    log_string( "Loading space..." );

    spaceobjectlist = str_printf("%s%s", SPACE_DIR, SPACE_LIST );
    FCLOSE( fpReserve );
    if ( ( fpList = fopen( spaceobjectlist.c_str(), "r" ) ) == NULL )
    {
	perror( spaceobjectlist.c_str() );
	exit( 1 );
    }

    for ( ; ; )
    {
	filename = feof( fpList ) ? "$" : fread_word( fpList );
	if ( filename[0] == '$' )
	  break;
	  
       
	if ( !load_spaceobject( game, filename ) )
	{
	  buf = str_printf("Cannot load spaceobject file: %s", filename);
	  bug( buf.c_str(), 0 );
	}
    }
    FCLOSE( fpList );
    log_string(" Done spaceobjects " );
    fpReserve = fopen( NULL_FILE, "r" );
    return;
}

void do_setspaceobject( CHAR_DATA *ch, char *argument )
{
    std::string arg1;
    std::string arg2;
    std::string argstr = argument;
    SPACE_DATA *spaceobject;

    if ( IS_NPC( ch ) )
    {
        send_to_char( "Huh?\n", ch );
        return;
    }

    argstr = one_argument( argstr, arg1 );
    argstr = one_argument( argstr, arg2 );

    if ( arg2.empty() || arg1.empty() )
    {
        send_to_char( "Usage: setspaceobject <spaceobject> <field> <values>\n", ch );
        send_to_char( "\nField being one of:\n", ch );
        send_to_char( "name filename type trainer,\n", ch );
        send_to_char( "xpos ypos zpos gravity seca secb secc,\n", ch );
        send_to_char( "locationa locationb locationc doca docb docc\n", ch );
        send_to_char( "", ch );
        return;
    }

    spaceobject = spaceobject_from_name( ch->game, arg1 );
    if ( !spaceobject )
    {
        send_to_char( "No such spaceobject.\n", ch );
        return;
    }


    if ( !str_cmp( arg2, "trainer" ) )
    {
      if ( spaceobject->trainer )
	      spaceobject->trainer = 0;
      else
        spaceobject->trainer = 1;
        
      send_to_char( "Done.\n", ch );
      save_spaceobject( spaceobject );
      return;
    }
    if ( !str_cmp( arg2, "seca" ) )
    {
      if ( spaceobject->seca )
	      spaceobject->seca = 0;
      else
        spaceobject->seca = 1;
        
      send_to_char( "Done.\n", ch );
      save_spaceobject( spaceobject );
      return;
    }
    if ( !str_cmp( arg2, "secb" ) )
    {
      if ( spaceobject->secb )
	      spaceobject->secb = 0;
      else
        spaceobject->secb = 1;
        
      send_to_char( "Done.\n", ch );
      save_spaceobject( spaceobject );
      return;
    }
    if ( !str_cmp( arg2, "secc" ) )
    {
      if ( spaceobject->secc )
	      spaceobject->secc = 0;
      else
        spaceobject->secc = 1;
        
      send_to_char( "Done.\n", ch );
      save_spaceobject( spaceobject );
      return;
    }
    if ( !str_cmp( arg2, "type" ) )
    {
        spaceobject->type = strtoi( argstr );
        send_to_char( "Done.\n", ch );
        save_spaceobject( spaceobject );
        return;
    }

    if ( !str_cmp( arg2, "doca" ) )
    {
        spaceobject->doca = strtoi( argstr );
        send_to_char( "Done.\n", ch );
        save_spaceobject( spaceobject );
        return;
    }
    if ( !str_cmp( arg2, "docb" ) )
    {
        spaceobject->docb = strtoi( argstr );
        send_to_char( "Done.\n", ch );
        save_spaceobject( spaceobject );
        return;
    }
    if ( !str_cmp( arg2, "docc" ) )
    {
        spaceobject->docc = strtoi( argstr );
        send_to_char( "Done.\n", ch );
        save_spaceobject( spaceobject );
        return;
    }


    if ( !str_cmp( arg2, "xpos" ) )
    {
        spaceobject->xpos = strtoi( argstr );
        send_to_char( "Done.\n", ch );
        save_spaceobject( spaceobject );
        return;
    }

    if ( !str_cmp( arg2, "ypos" ) )
    {
        spaceobject->ypos = strtoi( argstr );
        send_to_char( "Done.\n", ch );
        save_spaceobject( spaceobject );
        return;
    }
    
    if ( !str_cmp( arg2, "zpos" ) )
    {
        spaceobject->zpos = strtoi( argstr );
        send_to_char( "Done.\n", ch );
        save_spaceobject( spaceobject );
        return;
    }
    
    if ( !str_cmp( arg2, "gravity" ) )
    {
        spaceobject->gravity = strtoi( argstr );
        send_to_char( "Done.\n", ch );
        save_spaceobject( spaceobject );
        return;
    }
    if ( !str_cmp( arg2, "hx" ) )
    {
        spaceobject->hx = strtoi( argstr );
        send_to_char( "Done.\n", ch );
        save_spaceobject( spaceobject );
        return;
    }

    if ( !str_cmp( arg2, "hy" ) )
    {
        spaceobject->hy = strtoi( argstr );
        send_to_char( "Done.\n", ch );
        save_spaceobject( spaceobject );
        return;
    }
    
    if ( !str_cmp( arg2, "hz" ) )
    {
        spaceobject->hz = strtoi( argstr );
        send_to_char( "Done.\n", ch );
        save_spaceobject( spaceobject );
        return;
    }
    
    if ( !str_cmp( arg2, "speed" ) )
    {
        spaceobject->speed = strtoi( argstr );
        send_to_char( "Done.\n", ch );
        save_spaceobject( spaceobject );
        return;
    }
    
    if ( !str_cmp( arg2, "name" ) )
    {
        STRFREE( spaceobject->name );
        spaceobject->name = STRALLOC( argstr );
        send_to_char( "Done.\n", ch );
        save_spaceobject( spaceobject );
        return;
    }

    if ( !str_cmp( arg2, "filename" ) )
    {
        STR_DISPOSE( spaceobject->filename );
        spaceobject->filename = str_dup( argstr );
        send_to_char( "Done.\n", ch );
        save_spaceobject( spaceobject );
        return;
    }
    
    if ( !str_cmp( arg2, "locationa" ) )
    {
        STRFREE( spaceobject->locationa );
        spaceobject->locationa = STRALLOC( argstr );
        send_to_char( "Done.\n", ch );
        save_spaceobject( spaceobject );
        return;
    }
    if ( !str_cmp( arg2, "locationb" ) )
    {
        STRFREE( spaceobject->locationb );
        spaceobject->locationb = STRALLOC( argstr );
        send_to_char( "Done.\n", ch );
        save_spaceobject( spaceobject );
        return;
    }
    if ( !str_cmp( arg2, "locationc" ) )
    {
        STRFREE( spaceobject->locationc );
        spaceobject->locationc = STRALLOC( argstr );
        send_to_char( "Done.\n", ch );
        save_spaceobject( spaceobject );
        return;
    }

    do_setspaceobject( ch, "" );
    return;
}

void showspaceobject( CHAR_DATA *ch , SPACE_DATA *spaceobject )
{   
    ch_printf( ch, "Space object:%s     Trainer: %s	Filename: %s  Type %d\n",
    			spaceobject->name, 
    			(spaceobject->trainer ? "Yes" : "No"),
    			spaceobject->filename, spaceobject->type );
    ch_printf( ch, "Coordinates: %.0f %0.f %.0f\nGravity: %d\n",
    			spaceobject->xpos , spaceobject->ypos, spaceobject->zpos, spaceobject->gravity);
    ch_printf( ch, "     doca: %5d (%s)\n",
    			spaceobject->doca, spaceobject->locationa);
    ch_printf( ch, "     docb: %5d (%s)\n",
    			spaceobject->docb, spaceobject->locationb);
    ch_printf( ch, "     docc: %5d (%s)\n",
    			spaceobject->docc, spaceobject->locationc);
    return;
}

void do_showspaceobject( CHAR_DATA *ch, char *argument )
{
   SPACE_DATA *spaceobject;

   spaceobject = spaceobject_from_name( ch->game, argument );
   
   if ( spaceobject == NULL )
      send_to_char("&RNo such spaceobject.\n",ch);
   else
      showspaceobject(ch , spaceobject);
   
}

void do_makespaceobject( CHAR_DATA *ch, char *argument )
{   
    std::string arg;
    std::string filename;
    SPACE_DATA *spaceobject;

    if ( !argument || argument[0] == '\0' )
    {
      send_to_char( "Usage: makespaceobject <spaceobject name>\n", ch );
      return;
    }


    CREATE( spaceobject, SPACE_DATA, 1 );
    spaceobject->game = ch->game;
    LINK( spaceobject, first_spaceobject, last_spaceobject, next, prev );

    spaceobject->name		= STRALLOC( argument );
    
		  spaceobject->locationa            = STRALLOC( "" );  
		  spaceobject->locationb            = STRALLOC( "" );  
		  spaceobject->locationc            = STRALLOC( "" );  
    
    one_argument( argument, arg );
    filename = str_printf("%s.system" , strlower(arg).c_str() );
    spaceobject->filename = str_dup( filename );
    save_spaceobject( spaceobject );
    if ( !ch->game )
      log_printf( "do_makespaceobject: %s - No game set", ch->name );
    write_spaceobject_list(ch->game);
}

void do_spaceobjects( CHAR_DATA *ch, char *argument )
{
    SPACE_DATA *spaceobject;
    int count = 0;

    set_char_color( AT_RED, ch );

    for ( spaceobject = first_spaceobject; spaceobject; spaceobject = spaceobject->next )
    {
        if( spaceobject->type > SPACE_SUN )
          continue;
        if ( !(spaceobject->trainer && (!IS_GOD(ch))) )
          ch_printf( ch, "%s\n", spaceobject->name );
        count++;
    }

    ch_printf( ch, "\n" );

    set_char_color( AT_NOTE, ch );

    for ( spaceobject = first_spaceobject; spaceobject; spaceobject = spaceobject->next )
    {
        if( spaceobject->type != SPACE_PLANET )
          continue;
        if ( !(spaceobject->trainer && (!IS_GOD(ch))) )
          ch_printf( ch, "%s\n", spaceobject->name );
        count++;
    }

    if ( !count )
    {
        send_to_char( "There are no spaceobjects currently formed.\n", ch );
	      return;
    }
}
   
void echo_to_ship( int color , SHIP_DATA *ship , const std::string& argument )
{
     int room;
      
     for ( room = ship->get_firstroom() ; room <= ship->get_lastroom() ;room++ )
     {
         echo_to_room( color , get_room_index(room) , argument );
     }  
     
}

void sound_to_ship( SHIP_DATA *ship , const std::string& argument )
{
     int roomnum;
     ROOM_INDEX_DATA *room;
     CHAR_DATA *vic;
      
     for ( roomnum = ship->get_firstroom() ; roomnum <= ship->get_lastroom() ;roomnum++ )
     {
        room = get_room_index( roomnum );
        if ( room == NULL ) continue;
        
        for ( vic = room->first_person; vic; vic = vic->next_in_room )
        {
	   if ( !IS_NPC(vic) && BV_IS_SET( vic->act, PLR_SOUND ) )
	     send_to_char( argument, vic );
        }
     }  
     
}

void echo_to_docked( int color , SHIP_DATA *ship , const std::string& argument )
{
  SHIP_DATA *dship;
  
  for( dship = first_ship; dship; dship = dship->next )
    if( dship->docked && dship->docked == ship)
      echo_to_cockpit( color, dship, argument );
}

void echo_to_cockpit( int color , SHIP_DATA *ship , const std::string& argument )
{
     int room;
     TURRET_DATA *turret;      
     for ( room = ship->get_firstroom() ; room <= ship->get_lastroom() ;room++ )
     {
         if ( room == ship->cockpit || room == ship->navseat
         || room == ship->pilotseat || room == ship->coseat
         || room == ship->gunseat || room == ship->engineroom )
               echo_to_room( color , get_room_index(room) , argument );
         if( ship->first_turret )
           for( turret = ship->first_turret; turret; turret = turret->next )
             if( turret->roomvnum == room )
               echo_to_room( color , get_room_index(room) , argument );
     }  
}

bool ship_in_range( SHIP_DATA *ship, SHIP_DATA *target )
{
  if (target && ship && target != ship )
    if ( target->spaceobject && ship->spaceobject && 
         target->shipstate != SHIP_LANDED &&
         abs( (int) ( target->vx - ship->vx )) < 100*(ship->mod->sensor+10)*((target->shipclass == SHIP_DEBRIS ? 2 : target->shipclass)+3) &&
         abs( (int) ( target->vy - ship->vy )) < 100*(ship->mod->sensor+10)*((target->shipclass == SHIP_DEBRIS ? 2 : target->shipclass)+3) &&
         abs( (int) ( target->vz - ship->vz )) < 100*(ship->mod->sensor+10)*((target->shipclass == SHIP_DEBRIS ? 2 : target->shipclass)+3) )
      return TRUE;
  return FALSE;
}	

bool missile_in_range( SHIP_DATA *ship, MISSILE_DATA *missile, int range )
{
  if ( missile && ship )
    if ( ship->spaceobject && 
         abs( (int) ( missile->mx - ship->vx )) < range &&
         abs( (int) ( missile->my - ship->vy )) < range &&
         abs( (int) ( missile->mz - ship->vz )) < range )
      return TRUE;
  return FALSE;
}	

bool space_in_range( SHIP_DATA *ship, SPACE_DATA *object, int range )
{
  if (object && ship )
    if ( ship->spaceobject && 
         abs( (int) ( object->xpos - ship->vx )) < range &&
         abs( (int) ( object->ypos - ship->vy )) < range &&
         abs( (int) ( object->zpos - ship->vz )) < range )
      return TRUE;
  return FALSE;
}	
bool space_in_range_c( SHIP_DATA *ship, SPACE_DATA *object, int range )
{
  if (object && ship )
    if ( abs( (int) ( object->xpos - ship->vx )) < range &&
         abs( (int) ( object->ypos - ship->vy )) < range &&
         abs( (int) ( object->zpos - ship->vz )) < range )
      return TRUE;
  return FALSE;
}	
bool space_in_range_h( SHIP_DATA *ship, SPACE_DATA *object )
{
  if (object && ship )
    if ( abs( (int) ( object->xpos - ship->cx )) < (object->gravity*5) &&
         abs( (int) ( object->ypos - ship->cy )) < (object->gravity*5) &&
         abs( (int) ( object->zpos - ship->cz )) < (object->gravity*5) )
      return TRUE;
  return FALSE;
}	

char *print_distance_ship(SHIP_DATA *ship, SHIP_DATA *target)
{
    // Allocate a buffer for the string
    char *buf = (char *)malloc(64);  // enough for 3 floats + spaces + null
    if (!buf)
        return NULL;

    float hx = target->vx - ship->vx;
    float hy = target->vy - ship->vy;
    float hz = target->vz - ship->vz;

    // Use snprintf for safety
    snprintf(buf, 64, "%.0f %.0f %.0f", hx, hy, hz);

    return buf;  // caller must free()
}
void echo_to_system( int color , SHIP_DATA *ship , const std::string& argument , SHIP_DATA *ignore )
{
     SHIP_DATA *target;

     if (!ship->spaceobject)
        return;

     for ( target = first_ship; target; target = target->next )
     {
       if( !ship_in_range( ship, target ) )
         continue;
       if (target != ship && target != ignore )
         if ( space_distance_ship_less_than( ship, target, 100*(target->mod->sensor+10)*((ship->shipclass == SHIP_DEBRIS ? 2 : ship->shipclass)+1) ) )
           echo_to_cockpit( color , target , argument );
     }

}

bool is_facing( SHIP_DATA *ship , SHIP_DATA *target )
{
    float dy, dx, dz, hx, hy, hz;
      float cosofa;
          
    hx = ship->hx;
    hy = ship->hy;
    hz = ship->hz;
     	     				
    dx = target->vx - ship->vx;
    dy = target->vy - ship->vy;
    dz = target->vz - ship->vz;
     	     							
    cosofa = ( hx*dx + hy*dy + hz*dz ) 
      / ( sqrt(hx*hx+hy*hy+hz*hz) + sqrt(dx*dx+dy*dy+dz*dz) );
                                          
    if ( cosofa > 0.75 )
      return TRUE;
     	     								               	             
    return FALSE;
}
     	     								               	             	

long int get_ship_value( SHIP_DATA *ship )
{
     long int price;
     TURRET_DATA *turret;
          
     if (ship->shipclass == FIGHTER_SHIP)
        price = 5000;
     else if (ship->shipclass == MIDSIZE_SHIP)
     	price = 50000; 
     else if (ship->shipclass == CAPITAL_SHIP) 
        price = 500000;
     else 
        price = 2000;
        
     if ( ship->shipclass <= CAPITAL_SHIP ) 
       price += ( ship->mod->manuever*100*(1+ship->shipclass) );
     
     price += ( ship->mod->tractorbeam * 100 );
     price += ( ship->mod->realspeed * 10 );
     price += ( ship->mod->astro_array *5 );
     price += ( 5 * ship->mod->maxhull );
     price += ( 2 * ship->mod->maxenergy );

     if (ship->mod->maxenergy > 5000 )
          price += ( (ship->mod->maxenergy-5000)*20 ) ;
     
     if (ship->mod->maxenergy > 10000 )
          price += ( (ship->mod->maxenergy-10000)*50 );
     
     if (ship->mod->maxhull > 1000)
        price += ( (ship->mod->maxhull-1000)*10 );
     
     if (ship->mod->maxhull > 10000)
        price += ( (ship->mod->maxhull-10000)*20 );
        
     if (ship->mod->maxshield > 200)
          price += ( (ship->mod->maxshield-200)*50 );
     
     if (ship->mod->maxshield > 1000)
          price += ( (ship->mod->maxshield-1000)*100 );

     if (ship->mod->realspeed > 100 )
        price += ( (ship->mod->realspeed-100)*500 ) ;
        
     if (ship->mod->lasers > 5 )
        price += ( (ship->mod->lasers-5)*500 );
      
     if (ship->mod->maxshield)
     	price += ( 1000 + 10 * ship->mod->maxshield);
     
     if (ship->mod->lasers)
     	price += ( 500 + 500 * ship->mod->lasers );
    
     if (ship->mod->launchers )
     	price += ( 1000 + 100 * ship->mod->launchers);
     
     if (ship->missiles )
     	price += ( 250 * ship->missiles );
     else if (ship->torpedos )
     	price += ( 500 * ship->torpedos );
     else if (ship->rockets )
        price += ( 1000 * ship->rockets );
         
     if( ship->first_turret )
       for( turret = ship->first_turret; turret; turret = turret->next )
         price += 5000;
     
     if (ship->mod->hyperspeed)
        price += ( 1000 + ship->mod->hyperspeed * 10 );
     
     if (ship->hanger)
        price += ( ship->shipclass == MIDSIZE_SHIP ? 50000 : 100000 );
 
     price = (int) ( price*1.5 );
     
     return price;
     
}

void write_ship_list( GameContext *game )
{
    SHIP_DATA *tship;
    FILE *fpout;
    std::string filename;
    
    filename = str_printf("%s%s", SHIP_DIR, SHIP_LIST);
    fpout = fopen( filename.c_str(), "w" );
    if ( !fpout )
    {
         bug( "FATAL: cannot open ship.lst for writing!\n", 0 );
         return;
    }
    for ( tship = first_ship; tship; tship = tship->next )
    {
      if( tship->shipclass != SHIP_DEBRIS )
        fprintf( fpout, "%s\n", tship->filename );
    }
    fprintf( fpout, "$\n" );
    FCLOSE( fpout );
}
                                                                    
SHIP_DATA * ship_in_room( ROOM_INDEX_DATA *room, const std::string& name )
{
    SHIP_DATA *ship;

    if ( !room )
     return NULL;
     
    for ( ship = first_ship ; ship ; ship = ship->next )
    {
      if( ship->location == room->vnum )
      {
        if( ship->personalname )
          if ( !str_cmp( name, ship->personalname ) )
            return ship;
        if ( !str_cmp( name, ship->name ) )
          return ship;
      }
    }
    for ( ship = first_ship ; ship ; ship = ship->next )
    {
      if( ship->location == room->vnum )
      {
        if( ship->personalname )
          if ( ship->personalname && (ship->personalname[0] != '\0') && nifty_is_name_prefix( name, ship->personalname ) )
            return ship;
        if ( ship->name && (ship->name[0] != '\0') && nifty_is_name_prefix( name, ship->name ) )
          return ship;
      }
    }
    return NULL;    
}

/*
 * Get pointer to ship structure from ship name.
 */
SHIP_DATA *get_ship( GameContext *game, const std::string& name )
{
    SHIP_DATA *ship;
    

    for ( ship = first_ship; ship; ship = ship->next )
    {
      if( ship->personalname )
        if ( !str_cmp( name, ship->personalname ) )
            return ship;
      if ( !str_cmp( name, ship->name ) )
          return ship;
      if( ship->personalname )
        if ( ship->personalname && (ship->personalname[0] != '\0') && nifty_is_name_prefix( name, ship->personalname ) )
            return ship;
      if ( ship->name && (ship->name[0] != '\0') && nifty_is_name_prefix( name, ship->name ) )
          return ship;
    }
/*  for ( ship = first_ship; ship; ship = ship->next )
    {
    }
*/
    return NULL;
}

SHIP_DATA *get_ship_in_space( GameContext *game, const std::string& name )
{
    SHIP_DATA *ship;
    

    for ( ship = first_ship; ship; ship = ship->next )
    {
      if( (!ship->spaceobject || ship->shipstate == SHIP_LANDED) && ship->shipstate != SHIP_HYPERSPACE)
        continue;
      if( ship->personalname )
        if ( !str_cmp( name, ship->personalname ) )
            return ship;
      if ( !str_cmp( name, ship->name ) )
          return ship;
      if( ship->personalname )
        if ( ship->personalname && (ship->personalname[0] != '\0') && nifty_is_name_prefix( name, ship->personalname ) )
            return ship;
      if ( ship->name && (ship->name[0] != '\0') && nifty_is_name_prefix( name, ship->name ) )
          return ship;
    }
/*  for ( ship = first_ship; ship; ship = ship->next )
    {
    }
*/
    return NULL;
}

/*
 * Checks if ships in a spaceobject and returns poiner if it is.
 */
SHIP_DATA *get_ship_here( GameContext *game, const std::string& name , SHIP_DATA *eShip)
{
    SHIP_DATA *ship;
    int number, count = 0;
    std::string arg;

    if ( eShip == NULL )
         return NULL;

    number = number_argument( name, arg );
    
    for ( ship = first_ship ; ship; ship = ship->next )
    {
     if( !ship_in_range( eShip, ship ) )
       continue;
     if( !ship->spaceobject )
       continue;
     if( ship->personalname )
      if ( !str_cmp( arg, ship->personalname ) )
      {
       count++;
       if( !number || count == number )
         return ship;
     }
     if ( !str_cmp( arg, ship->name ) )
     {
       count++;
       if( !number ||  count == number )
         return ship;
     }
    }
    count = 0;
    for ( ship = first_ship; ship; ship = ship->next )
    {
     if( !ship_in_range( eShip, ship ) )
       continue;
     if( ship->personalname )
      if ( ship->personalname && (ship->personalname[0] != '\0') && nifty_is_name_prefix( arg, ship->personalname ) )
      {
       count++;
       if(  !number || count == number )
         return ship;
     }
     if ( ship->name && (ship->name[0] != '\0') && nifty_is_name_prefix( arg, ship->name ) )
     {
       count++;
       if(  !number || count == number )
         return ship;
     }
    }
    return NULL;
}

/*
 * Get pointer to ship structure from ship filename.
 */
SHIP_DATA *get_ship_from_filename( GameContext *game, const std::string& name )
{
    SHIP_DATA *ship;
    

    for ( ship = first_ship; ship; ship = ship->next )
    {
     if ( !str_cmp( name, ship->filename ) )
         return ship;
     if ( ship->filename && (ship->filename[0] != '\0') && nifty_is_name_prefix( name, ship->filename) )
         return ship;
    }
/*  for ( ship = first_ship; ship; ship = ship->next )
    {
    }
*/
    return NULL;
}

/*
 * Get pointer to ship structure from pilot name.
 */
SHIP_DATA *ship_from_pilot( GameContext *game, const std::string& name )
{
    SHIP_DATA *ship;
    
    for ( ship = first_ship; ship; ship = ship->next )
       if ( !str_cmp( name, ship->pilot ) )
         return ship;
    if ( !str_cmp( name, ship->copilot ) )
      return ship;
    if ( !str_cmp( name, ship->owner ) )
      return ship;  
    return NULL;
}

/*
 * Get pointer to # ship structure from pilot name.
 */
SHIP_DATA *ship_from_pilot_num( GameContext *game, const std::string& name, int num )
{
    SHIP_DATA *ship;
    int count = 1;
    
    if ( !num )
      num = 1;
    for ( ship = first_ship; ship; ship = ship->next )
    {
       if ( !str_cmp( name, ship->pilot ) && count == num )
         return ship;
       if ( !str_cmp( name, ship->copilot ) && count == num )
         return ship;
       if ( !str_cmp( name, ship->owner ) && count == num )
         return ship;  
       count++;
    }
    return NULL;
}

/*
 * Get pointer to ship structure from cockpit, turret, or entrance ramp vnum.
 */
 
SHIP_DATA *ship_from_cockpit( GameContext *game, int vnum )
{
    SHIP_DATA *ship;
    TURRET_DATA *turret;
    
    for ( ship = first_ship; ship; ship = ship->next )
    {
       if ( vnum == ship->cockpit || vnum == ship->hanger
          || vnum == ship->pilotseat || vnum == ship->coseat || vnum == ship->navseat
          || vnum == ship->gunseat  || vnum == ship->engineroom )
         return ship;
      if( ship->first_turret )
        for( turret = ship->first_turret; turret; turret = turret->next )
          if( turret->roomvnum == vnum )
            return ship;
      if( ship->cockpitroom && ship->cockpitroom->vnum == vnum )
        return ship;
    }
    return NULL;
}

SHIP_DATA *ship_from_pilotseat( GameContext *game, int vnum )
{
    SHIP_DATA *ship;

    for ( ship = first_ship; ship; ship = ship->next )
    {
       if ( vnum == ship->pilotseat )
         return ship;
      if( ship->cockpitroom && ship->cockpitroom->vnum == vnum )
        return ship;
    }

    return NULL;
}

SHIP_DATA *ship_from_coseat( GameContext *game, int vnum )
{
    SHIP_DATA *ship;
    
    for ( ship = first_ship; ship; ship = ship->next )
    {
      if ( vnum == ship->coseat )
         return ship;
      if( ship->cockpitroom && ship->cockpitroom->vnum == vnum )
        return ship;
    }

    return NULL;
}

SHIP_DATA *ship_from_navseat( GameContext *game, int vnum )
{
    SHIP_DATA *ship;
    
    for ( ship = first_ship; ship; ship = ship->next )
    {
      if ( vnum == ship->navseat )
         return ship;
      if( ship->cockpitroom && ship->cockpitroom->vnum == vnum )
        return ship;
    }
    
    return NULL;
}

SHIP_DATA *ship_from_gunseat( GameContext *game, int vnum )
{
    SHIP_DATA *ship;
    
    for ( ship = first_ship; ship; ship = ship->next )
    {
      if ( vnum == ship->gunseat )
        return ship;
      if( ship->cockpitroom && ship->cockpitroom->vnum == vnum )
        return ship;
    }
    return NULL;
}

SHIP_DATA *ship_from_engine( GameContext *game, int vnum )
{
    SHIP_DATA *ship;
    TURRET_DATA *turret;
    
    for ( ship = first_ship; ship; ship = ship->next )
    {   
      if (ship->engineroom)   
      {  
        if ( vnum == ship->engineroom )
          return ship;
      }
      else
      { 
        if ( vnum == ship->cockpit )
          return ship;
      }
      if ( ship->first_turret )
        for ( turret = ship->first_turret; turret; turret = turret->next )
          if ( vnum == turret->roomvnum )
            return ship;
      if( ship->cockpitroom && ship->cockpitroom->vnum == vnum )
        return ship;
    }

    return NULL;
}



SHIP_DATA *ship_from_turret( GameContext *game, int vnum )
{
    SHIP_DATA *ship;
    TURRET_DATA *turret;
    
    for ( ship = first_ship; ship; ship = ship->next )
    {
       if ( vnum == ship->gunseat )
         return ship;
      if( ship->first_turret )
        for( turret = ship->first_turret; turret; turret = turret->next )
          if( turret->roomvnum == vnum )
            return ship;
      if( ship->cockpitroom && ship->cockpitroom->vnum == vnum )
        return ship;
    }

    return NULL;
}

SHIP_DATA *ship_from_entrance( GameContext *game, int vnum )
{
    SHIP_DATA *ship;
    
    for ( ship = first_ship; ship; ship = ship->next )
    {
       if ( vnum == ship->entrance )
         return ship;
      if( ship->cockpitroom && ship->cockpitroom->vnum == vnum )
        return ship;
    }
    return NULL;
}

SHIP_DATA *ship_from_hanger( GameContext *game, int vnum )
{
    SHIP_DATA *ship;
    
    if ( vnum )
     for ( ship = first_ship; ship; ship = ship->next )
       if ( vnum == ship->hanger )
         return ship;
    return NULL;
}

SHIP_DATA *ship_from_room( GameContext *game, int vnum )
{
//  SHIP_DATA *ship;
    ROOM_INDEX_DATA *room;
    
     room = get_room_index( vnum );
    if (room && room->ship)
      return room->ship;

    return NULL;
        /*
    for ( ship = first_ship; ship; ship = ship->next )
       if ( vnum >= ship->get_firstroom() && vnum <= ship->get_lastroom() )
         return ship;
    return NULL;
    */
}

void save_ship( SHIP_DATA *ship )
{
    FILE *fp;
    std::string filename;
    std::string buf;
    MODULE_DATA *module;
    TURRET_DATA *turret;

    if ( !ship )
    {
	bug( "save_ship: null ship pointer!", 0 );
	return;
    }
    
    update_ship_modules( ship );
    
    if ( ship->shipclass == SHIP_DEBRIS || ship->cockpitroom )
      return;
        
    if ( !ship->filename || ship->filename[0] == '\0' )
    {
	buf = str_printf("save_ship: %s has no filename", ship->name);
	bug( buf.c_str(), 0 );
	return;
    }

    filename = str_printf("%s%s", SHIP_DIR, ship->filename);
    
    FCLOSE( fpReserve );
    if ( ( fp = fopen( filename.c_str(), "w" ) ) == NULL )
    {
    	bug( "save_ship: fopen", 0 );
    	perror( filename.c_str() );
    }
    else
    {
	fprintf( fp, "#SHIP\n" );
	fprintf( fp, "Name         %s~\n",	ship->name		);
	fprintf( fp, "PersonalName         %s~\n",	ship->personalname		);
	fprintf( fp, "Filename     %s~\n",	ship->filename		);
	fprintf( fp, "ShipID	   %ld\n",	ship->shipID		);
        fprintf( fp, "Description  %s~\n",	ship->description	);
	fprintf( fp, "Owner        %s~\n",	ship->owner		);
	fprintf( fp, "Pilot        %s~\n",      ship->pilot             );
	fprintf( fp, "Copilot      %s~\n",      ship->copilot           );
	fprintf( fp, "Class        %d\n",	ship->shipclass		);
	fprintf( fp, "Tractorbeam  %d\n",	ship->tractorbeam	);
	fprintf( fp, "Shipyard     %d\n",	ship->shipyard		);
	fprintf( fp, "Hanger       %d\n",	ship->hanger    	);
//	fprintf( fp, "Cargohold       %d\n",	ship->cargohold		);
	fprintf( fp, "Vx           %.0f\n",	ship->vx        	);
	fprintf( fp, "Vy           %.0f\n",	ship->vy        	);
	fprintf( fp, "Vz           %.0f\n",	ship->vz        	);
/*	fprintf( fp, "Turret1      %d\n",	ship->turret1		);
	fprintf( fp, "Turret2      %d\n",	ship->turret2		);
	fprintf( fp, "Turret3      %d\n",	ship->turret3		);
	fprintf( fp, "Turret4      %d\n",	ship->turret4		);
	fprintf( fp, "Turret5      %d\n",	ship->turret5		);
	fprintf( fp, "Turret6      %d\n",	ship->turret6		);
	fprintf( fp, "Turret7      %d\n",	ship->turret7		);
	fprintf( fp, "Turret8      %d\n",	ship->turret8		);
	fprintf( fp, "Turret9      %d\n",	ship->turret9		);
	fprintf( fp, "Turret0      %d\n",	ship->turret0		);
*/
	fprintf( fp, "Statet0      %d\n",	ship->statet0		);
	fprintf( fp, "Statei0      %d\n",	ship->statei0		);
	fprintf( fp, "Lasers       %d\n",	ship->lasers    	);
	fprintf( fp, "Missiles     %d\n",	ship->missiles		);
	fprintf( fp, "Maxmissiles  %d\n",	ship->maxmissiles	);
	fprintf( fp, "Rockets     %d\n",	ship->rockets		);
	fprintf( fp, "Maxrockets  %d\n",	ship->maxrockets	);
	fprintf( fp, "Torpedos     %d\n",	ship->torpedos		);
	fprintf( fp, "Maxtorpedos  %d\n",	ship->maxtorpedos	);
	fprintf( fp, "Lastdoc      %d\n",	ship->lastdoc		);
	fprintf( fp, "Firstroom    %d\n",	ship->get_firstroom()		);
	fprintf( fp, "Lastroom     %d\n",	ship->get_lastroom()		);
	fprintf( fp, "Shield       %d\n",	ship->shield		);
	fprintf( fp, "Maxshield    %d\n",	ship->maxshield		);
	fprintf( fp, "Hull         %d\n",	ship->hull		);
	fprintf( fp, "Maxhull      %d\n",	ship->maxhull		);
	fprintf( fp, "Maxenergy    %d\n",	ship->maxenergy		);
	fprintf( fp, "Hyperspeed   %d\n",	ship->hyperspeed	);
	fprintf( fp, "Comm         %d\n",	ship->comm		);
	fprintf( fp, "Chaff        %d\n",	ship->chaff		);
	fprintf( fp, "Maxchaff     %d\n",	ship->maxchaff		);
	fprintf( fp, "Sensor       %d\n",	ship->sensor		);
	fprintf( fp, "Astro_array  %d\n",	ship->astro_array	);
	fprintf( fp, "Realspeed    %d\n",	ship->realspeed		);
	fprintf( fp, "Type         %d\n",	ship->type		);
	fprintf( fp, "Cockpit      %d\n",	ship->cockpit		);
	fprintf( fp, "Coseat       %d\n",	ship->coseat		);
	fprintf( fp, "Pilotseat    %d\n",	ship->pilotseat		);
	fprintf( fp, "Gunseat      %d\n",	ship->gunseat		);
	fprintf( fp, "Navseat      %d\n",	ship->navseat		);
	fprintf( fp, "Engineroom   %d\n",       ship->engineroom        );
	fprintf( fp, "Entrance     %d\n",       ship->entrance          );
	fprintf( fp, "Shipstate    %d\n",	ship->shipstate		);
	fprintf( fp, "Missilestate %d\n",	ship->missilestate	);
	fprintf( fp, "Energy       %d\n",	ship->energy		);
	fprintf( fp, "Manuever     %d\n",       ship->manuever          );
	fprintf( fp, "Alarm        %d\n",       ship->alarm             );
	fprintf( fp, "UpgradeBlock %d\n",	ship->upgradeblock    	);
   fprintf( fp, "Ions         %d\n",       ship->ions              );
   fprintf( fp, "MaxIntModules   %d\n",		ship->maxintmodules    	);
   fprintf( fp, "MaxExtModules   %d\n",		ship->maxextmodules    	);
   fprintf( fp, "Maxcargo     %d\n",       ship->maxcargo              );
   fprintf( fp, "Weight         %d\n",       ship->weight              );
/* fprintf( fp, "Cargo0         %d\n",       ship->cargo0              );
   fprintf( fp, "Cargo1         %d\n",       ship->cargo1              );
   fprintf( fp, "Cargo2         %d\n",       ship->cargo2              );
   fprintf( fp, "Cargo3         %d\n",       ship->cargo3              );
   fprintf( fp, "Cargo4         %d\n",       ship->cargo4              );
   fprintf( fp, "Cargo5         %d\n",       ship->cargo5              );
   fprintf( fp, "Cargo6         %d\n",       ship->cargo6              );
   fprintf( fp, "Cargo7         %d\n",       ship->cargo7              );
   fprintf( fp, "Cargo8         %d\n",       ship->cargo8              );
   fprintf( fp, "Cargo9         %d\n",       ship->cargo9              );
   fprintf( fp, "CaughtSmug     %d\n",       ship->caughtsmug              );
*/
   fprintf( fp, "Dockingports   %d\n",       ship->dockingports        );
   fprintf( fp, "Guard   %d\n",		     ship->guard        );
   fprintf( fp, "Home         %s~\n",      ship->home              );
   if( !ship->templatestring )
     ship->templatestring = STRALLOC( "" );
   fprintf( fp, "Templatestring %s~\n",	     ship->templatestring );
   fprintf( fp, "End\n"						);
   if ( ship->first_module)
   {
		for( module = ship->first_module; module; module = module->next )
		{
			fprintf( fp, "#MODS\n"					);
			fprintf( fp, "Name %s~\nMod %d %d %d %d~\n", 
			   module->name, module->type, module->condition, module->size, module->modification);	
			fprintf( fp, "End\n"					);
		}
   }

   if ( ship->first_turret )
   {
		for( turret = ship->first_turret; turret; turret = turret->next )
		{
			fprintf( fp, "#MODS\n"					);
			fprintf( fp, "Name %s~\nMod %d %d %d %d~\n", 
			   "Turret", MOD_TURRET, turret->type, turret->roomvnum, turret->state );	
			fprintf( fp, "End\n"					);
		}
   }


  }
    
  /*
    if ( ship->cargohold )
    {
       	OBJ_DATA *contents;
	ROOM_INDEX_DATA *cargoroom;
  	cargoroom = get_room_index(ship->cargohold);

    	if ( cargoroom != NULL )
    	{
        	contents = cargoroom->last_content;
//		fprintf( fp, "#CARGO\n" );

        	if (contents)
	  	  fwrite_obj(NULL, contents, fp, 0, OS_CARRY );
		fprintf( fp, "END\n" );
		fprintf( fp, "#END\n"						);
    	}
    }
*/    
    
    FCLOSE( fp );
    fpReserve = fopen( NULL_FILE, "r" );
    ship->lastsaved = 0;
    ship->dirty = FALSE;
    return;
}


/*
 * Read in actual ship data.
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

#if 0
void fread_cargohold( SHIP_DATA *ship, FILE *fp )
{

         ROOM_INDEX_DATA *storeroom = get_room_index(ship->cargohold);
         OBJ_DATA *obj;
         OBJ_DATA *obj_next;
           
	 if( !storeroom )
	   return;
	   
         for ( obj = storeroom->first_content; obj; obj = obj_next )
	 {
	    obj_next = obj->next_content;
	    extract_obj( obj );
	 }

	 if ( fp != NULL )
	 {
	    int iNest;
	    bool found;
	    OBJ_DATA *tobj, *tobj_next;

	    rset_supermob(storeroom);

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
		    bug( "fread_cargohold: # not found.", 0 );
		    bug( &letter, 0 );
		    bug( ship->name, 0 );
		    break;
		}

		word = fread_word( fp );
		if ( !str_cmp( word, "OBJECT" ) )	/* Objects	*/
		{
 		  bug( "Read an object!.", 0 );
		  fread_obj  ( supermob, fp, OS_CARRY );
		}
		else
		if ( !str_cmp( word, "END"    ) )	/* Done		*/
		  break;
		else
		{
		    bug( "fread_cargohold: bad section.", 0 );
		    bug( ship->name, 0 );
		    break;
		}
	    }

	    FCLOSE( fp );

	    for ( tobj = supermob->first_carrying; tobj; tobj = tobj_next )
	    {
		tobj_next = tobj->next_content;
		obj_from_char( tobj );
                if( tobj->item_type != ITEM_MONEY )
		  obj_to_room( tobj, storeroom );
	    }
	    
	    release_supermob(ship->game);

         }
}
#endif

void fread_ship( SHIP_DATA *ship, FILE *fp )
{
    std::string buf;
    const char *word;
    bool fMatch;
    int turretvnum;//, dummy_number;
    SHIP_MOD_DATA *ship_mod;
    TURRET_DATA *turret;

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
             KEY( "Astro_array",      ship->astro_array,       fread_number( fp ) );
             KEY( "Alarm",            ship->alarm,             fread_number( fp ) );
             break;
        
        case 'C':
/*           KEY( "Cargo0",  ship->cargo0, fread_number( fp ) );
             KEY( "Cargo1",  ship->cargo1, fread_number( fp ) );
             KEY( "Cargo2",  ship->cargo2, fread_number( fp ) );
             KEY( "Cargo3",  ship->cargo3, fread_number( fp ) );
             KEY( "Cargo4",  ship->cargo4, fread_number( fp ) );
             KEY( "Cargo5",  ship->cargo5, fread_number( fp ) );
             KEY( "Cargo6",  ship->cargo6, fread_number( fp ) );
             KEY( "Cargo7",  ship->cargo7, fread_number( fp ) );
             KEY( "Cargo8",  ship->cargo8, fread_number( fp ) );
             KEY( "Cargo9",  ship->cargo9, fread_number( fp ) );
             KEY( "Cargohold",  ship->cargohold, fread_number( fp ) );
             KEY( "CaughtSmug",  ship->caughtsmug, fread_number( fp ) );
*/
             KEY( "Cockpit",     ship->cockpit,          fread_number( fp ) );
             KEY( "Coseat",     ship->coseat,          fread_number( fp ) );
             KEY( "Class",       ship->shipclass,            fread_number( fp ) );
             KEY( "Copilot",     ship->copilot,          fread_string( fp ) );
             KEY( "Comm",        ship->comm,      fread_number( fp ) );
             KEY( "Chaff",       ship->chaff,      fread_number( fp ) );
             break;
                                

	case 'D':
	    KEY( "Description",	ship->description,	fread_string( fp ) );
	    KEY( "DockingPorts",    ship->dockingports,      fread_number( fp ) );
	    break;

	case 'E':
	    KEY( "Engineroom",    ship->engineroom,      fread_number( fp ) );
	    KEY( "Entrance",	ship->entrance,	        fread_number( fp ) );
	    KEY( "Energy",      ship->energy,        fread_number( fp ) );
	    if ( !str_cmp( word, "End" ) )
	    {
		if (!ship->home)
		  ship->home		= STRALLOC( "" );
		if (!ship->name)
		  ship->name		= STRALLOC( "" );
		if (!ship->owner)
		  ship->owner		= STRALLOC( "" );
		if (!ship->description)
		  ship->description 	= STRALLOC( "" );
		if (!ship->copilot)
		  ship->copilot 	= STRALLOC( "" );
		if (!ship->pilot)
		  ship->pilot   	= STRALLOC( "" );
		if (ship->shipstate != SHIP_DISABLED)
		  ship->shipstate = SHIP_LANDED;
		if (ship->statet0 != LASER_DAMAGED)
		  ship->statet0 = LASER_READY;
		if (ship->statei0 != LASER_DAMAGED)
		  ship->statei0 = LASER_READY;
		if (ship->missilestate != MISSILE_DAMAGED)
		  ship->missilestate = MISSILE_READY;
		if (ship->shipyard <= 0)
		  ship->shipyard = ROOM_LIMBO_SHIPYARD;
		if (ship->lastdoc <= 0) 
		  ship->lastdoc = ship->shipyard;
		ship->bayopen     = FALSE;
		ship->autopilot   = FALSE;
		ship->hatchopen = FALSE;
		ship->tractored    = NULL;
		ship->tractoredby = NULL;
		if (ship->navseat <= 0)
		  ship->navseat = ship->cockpit;
		if (ship->gunseat <= 0)
		  ship->gunseat = ship->cockpit;
		if (ship->coseat <= 0) 
		  ship->coseat = ship->cockpit;
		if (ship->pilotseat <= 0)
		  ship->pilotseat = ship->cockpit;
		if (ship->missiletype == 1)
		{
		  ship->torpedos = ship->missiles;    /* for back compatability */
		  ship->missiles = 0;
		}
		ship->spaceobject = NULL;
		ship->in_room=NULL;
                ship->next_in_room=NULL;
                ship->prev_in_room=NULL;
		ship->tcount = 0;
		ship->modules=0;
		if (ship->maxextmodules == 0)
			ship->maxextmodules = 10;
		if (ship->maxintmodules == 0)
			ship->maxintmodules = 3;
		
		if( ship->mod == NULL )
		{
 		  CREATE( ship_mod, SHIP_MOD_DATA, 1 );
		  ship->mod = ship_mod;
		  update_ship_modules(ship);
		}
			
    if( ship->shipclass < 3 )
            ship->bayopen = FALSE;
            
    if ( !ship->templatestring )
      ship->templatestring = STRALLOC( "" );
      
    if( ship->templatestring[0] != '\0' )
        if ( parse_ship_template(ship->templatestring, ship) )
		      bug( "fread_ship: parse_ship_template failed.\n", 0 );

		return;
    }
	  break;

	case 'F':
	    KEY( "Filename",	ship->filename,		fread_string_nohash( fp ) );
        if ( !str_cmp( word, "Firstroom" ) )
        {
            ship->set_firstroom(fread_number( fp ));
            fMatch = TRUE;
            break;
        }
            //KEY( "Firstroom",   ship->firstroom,        fread_number( fp ) );
            break;
        
        case 'G':    
            KEY( "Guard",     ship->guard,          fread_number( fp ) );
            KEY( "Gunseat",     ship->gunseat,          fread_number( fp ) );
            break;
        
        case 'H':
            KEY( "Home" , ship->home, fread_string( fp ) );
            KEY( "Hyperspeed",   ship->hyperspeed,      fread_number( fp ) );
            KEY( "Hull",      ship->hull,        fread_number( fp ) );
            KEY( "Hanger",  ship->hanger,      fread_number( fp ) );
            break;

        case 'I':
            KEY( "Ions" , ship->ions, fread_number( fp ) );
            break;

        case 'L':
            KEY( "Laserstr",   ship->lasers,   (sh_int)  ( fread_number( fp )/10 ) );
            KEY( "Lasers",   ship->lasers,      fread_number( fp ) );
            KEY( "Lastdoc",    ship->lastdoc,       fread_number( fp ) );
//          KEY( "Lastroom",   ship->lastroom,        fread_number( fp ) );
            if ( !str_cmp( word, "Lastroom" ) )
            {
                ship->set_lastroom(fread_number( fp ));
                fMatch = TRUE;
                break;
            }            
            break;

        case 'M':
            KEY( "Manuever",   ship->manuever,      fread_number( fp ) );
            KEY( "Maxcargo",   ship->maxcargo,      fread_number( fp ) );
	    KEY( "MaxIntModules",		ship->maxintmodules,	fread_number( fp ) );
	    KEY( "MaxExtModules",		ship->maxextmodules,	fread_number( fp ) );
            KEY( "Maxmissiles",   ship->maxmissiles,      fread_number( fp ) );
            KEY( "Maxtorpedos",   ship->maxtorpedos,      fread_number( fp ) );
            KEY( "Maxrockets",   ship->maxrockets,      fread_number( fp ) );
            KEY( "Missiles",   ship->missiles,      fread_number( fp ) );
            KEY( "Missiletype",   ship->missiletype,      fread_number( fp ) );
            KEY( "Maxshield",      ship->maxshield,        fread_number( fp ) );
            KEY( "Maxenergy",      ship->maxenergy,        fread_number( fp ) );
            KEY( "Missilestate",   ship->missilestate,        fread_number( fp ) );
            KEY( "Maxhull",      ship->maxhull,        fread_number( fp ) );
            KEY( "Maxchaff",       ship->maxchaff,      fread_number( fp ) );
             break;

	case 'N':
	    KEY( "Name",	ship->name,		fread_string( fp ) );
	    KEY( "Navseat",     ship->navseat,          fread_number( fp ) );
            break;
  
        case 'O':
            KEY( "Owner",            ship->owner,            fread_string( fp ) );
//            KEY( "Objectnum",        dummy_number,        fread_number( fp ) );
            break;

        case 'P':
	    KEY( "PersonalName",	ship->personalname,		fread_string( fp ) );
            KEY( "Pilot",            ship->pilot,            fread_string( fp ) );
            KEY( "Pilotseat",     ship->pilotseat,          fread_number( fp ) );
            break;
        
        case 'R':
            KEY( "Realspeed",   ship->realspeed,       fread_number( fp ) );
            KEY( "Rockets",     ship->rockets,         fread_number( fp ) );
            break;
       
        case 'S':
            KEY( "Shipyard",    ship->shipyard,      fread_number( fp ) );
            KEY( "Sensor",      ship->sensor,       fread_number( fp ) );
            KEY( "Shield",      ship->shield,        fread_number( fp ) );
            KEY( "ShipID",      ship->shipID,        fread_number( fp ) );
            KEY( "Shipstate",   ship->shipstate,        fread_number( fp ) );
            KEY( "Statei0",   ship->statet0,        fread_number( fp ) );
            KEY( "Statet0",   ship->statet0,        fread_number( fp ) );
            break;

	case 'T':
	    KEY( "Templatestring", ship->templatestring, fread_string( fp ) );
	    KEY( "Torpedos",	ship->torpedos,	fread_number( fp ) );
	    KEY( "Type",	ship->type,	fread_number( fp ) );
	    KEY( "Tractorbeam", ship->tractorbeam,      fread_number( fp ) );
	    if ( !str_cmp( word, "Turret1" ) )
		{
		  turretvnum = fread_number( fp );
		  if( turretvnum == 0 ) break;
		  CREATE( turret, TURRET_DATA, 1 );
		  turret->type	 	= 0;
		  turret->roomvnum	= turretvnum;
		  turret->state		= LASER_READY;

		  LINK( turret, ship->first_turret, ship->last_turret, next, prev );
		}
	    if ( !str_cmp( word, "Turret2" ) )
		{
		  turretvnum = fread_number( fp );
		  if( turretvnum == 0 ) break;
		  CREATE( turret, TURRET_DATA, 1 );
		  turret->type	 	= 0;
		  turret->roomvnum	= turretvnum;
		  turret->state		= LASER_READY;

		  LINK( turret, ship->first_turret, ship->last_turret, next, prev );
		}
	    if ( !str_cmp( word, "Turret3" ) )
		{
		  turretvnum = fread_number( fp );
		  if( turretvnum == 0 ) break;
		  CREATE( turret, TURRET_DATA, 1 );
		  turret->type	 	= 0;
		  turret->roomvnum	= turretvnum;
		  turret->state		= LASER_READY;

		  LINK( turret, ship->first_turret, ship->last_turret, next, prev );
		}
	    if ( !str_cmp( word, "Turret4" ) )
		{
		  turretvnum = fread_number( fp );
		  if( turretvnum == 0 ) break;
		  CREATE( turret, TURRET_DATA, 1 );
		  turret->type	 	= 0;
		  turret->roomvnum	= turretvnum;
		  turret->state		= LASER_READY;

		  LINK( turret, ship->first_turret, ship->last_turret, next, prev );
		}
	    if ( !str_cmp( word, "Turret5" ) )
		{
		  turretvnum = fread_number( fp );
		  if( turretvnum == 0 ) break;
		  CREATE( turret, TURRET_DATA, 1 );
		  turret->type	 	= 0;
		  turret->roomvnum	= turretvnum;
		  turret->state		= LASER_READY;

		  LINK( turret, ship->first_turret, ship->last_turret, next, prev );
		}
	    if ( !str_cmp( word, "Turret6" ) )
		{
		  turretvnum = fread_number( fp );
		  if( turretvnum == 0 ) break;
		  CREATE( turret, TURRET_DATA, 1 );
		  turret->type	 	= 0;
		  turret->roomvnum	= turretvnum;
		  turret->state		= LASER_READY;

		  LINK( turret, ship->first_turret, ship->last_turret, next, prev );
		}
	    if ( !str_cmp( word, "Turret7" ) )
		{
		  turretvnum = fread_number( fp );
		  if( turretvnum == 0 ) break;
		  CREATE( turret, TURRET_DATA, 1 );
		  turret->type	 	= 0;
		  turret->roomvnum	= turretvnum;
		  turret->state		= LASER_READY;

		  LINK( turret, ship->first_turret, ship->last_turret, next, prev );
		}
	    if ( !str_cmp( word, "Turret8" ) )
		{
		  turretvnum = fread_number( fp );
		  if( turretvnum == 0 ) break;
		  CREATE( turret, TURRET_DATA, 1 );
		  turret->type	 	= 0;
		  turret->roomvnum	= turretvnum;
		  turret->state		= LASER_READY;

		  LINK( turret, ship->first_turret, ship->last_turret, next, prev );
		}
	    if ( !str_cmp( word, "Turret9" ) )
		{
		  turretvnum = fread_number( fp );
		  if( turretvnum == 0 ) break;
		  CREATE( turret, TURRET_DATA, 1 );
		  turret->type	 	= 0;
		  turret->roomvnum	= turretvnum;
		  turret->state		= LASER_READY;

		  LINK( turret, ship->first_turret, ship->last_turret, next, prev );
		}
	    if ( !str_cmp( word, "Turret0" ) )
		{
		  turretvnum = fread_number( fp );
		  if( turretvnum == 0 ) break;
		  CREATE( turret, TURRET_DATA, 1 );
		  turret->type	 	= 0;
		  turret->roomvnum	= turretvnum;
		  turret->state		= LASER_READY;

		  LINK( turret, ship->first_turret, ship->last_turret, next, prev );
		}

/*	    KEY( "Turret1",	ship->turret1,	fread_number( fp ) );
	    KEY( "Turret2",	ship->turret2,	fread_number( fp ) );
       KEY( "Turret3",  ship->turret3, fread_number( fp ) );
       KEY( "Turret4",  ship->turret4, fread_number( fp ) );
       KEY( "Turret5",  ship->turret5, fread_number( fp ) );
       KEY( "Turret6",  ship->turret6, fread_number( fp ) );
       KEY( "Turret7",  ship->turret7, fread_number( fp ) );
       KEY( "Turret8",  ship->turret8, fread_number( fp ) );
       KEY( "Turret9",  ship->turret9, fread_number( fp ) );
       KEY( "Turret0",  ship->turret0, fread_number( fp ) );
*/
	    break;
	
	case 'U':
	    KEY( "UpgradeBlock",          ship->upgradeblock,     fread_number( fp ) );
	    break;
	
	case 'V':
	    KEY( "Vx",          ship->vx,     fread_number( fp ) );
	    KEY( "Vy",          ship->vy,     fread_number( fp ) );
	    KEY( "Vz",          ship->vz,     fread_number( fp ) );
	    break;

	case 'W':
	    KEY( "Weight",          ship->weight,     fread_number( fp ) );
	    break;
	}
	
	if ( !fMatch )
	{
	    buf = str_printf("Fread_ship: no match: %s", word);
	    bug( buf.c_str(), 0 );
	}
    }
}

// Helper to link ship rooms to the ship after loading a ship file.
void link_ship_rooms( GameContext *game, SHIP_DATA *ship )
{
    if ( !ship )
        return;

    for ( int vnum = ship->get_firstroom(); vnum <= ship->get_lastroom(); ++vnum )
    {
        ROOM_INDEX_DATA *room = get_room_index( vnum );
        if ( room )
            room->ship = ship;
    }
}
/*
 * Load a ship file
 */

bool load_ship_file( GameContext *game, const std::string& shipfile )
{
    std::string filename;
    SHIP_DATA *ship;
    FILE *fp;
    bool found, isbus = FALSE;
    int bus;
    ROOM_INDEX_DATA *pRoomIndex;
    CLAN_DATA *clan;

    CREATE( ship, SHIP_DATA, 1 );
    ship->game = game;
    found = FALSE;
    filename = str_printf("%s%s", SHIP_DIR, shipfile.c_str() );

    if ( ( fp = fopen( filename.c_str(), "r" ) ) != NULL )
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
		bug( "Load_ship_file: # not found.", 0 );
		break;
	    }

	    word = fread_word( fp );
	    if ( !str_cmp( word, "SHIP"	) )
	    {
	    	fread_ship( ship, fp );
//		fread_cargohold( ship, fp );
	    }
	    else if ( !str_cmp( word, "MODS" ) )	
		{		
//			log_string( "Read Modules" );
			fread_modules( ship, fp );
			update_ship_modules( ship );
		}
	    else
	    if ( !str_cmp( word, "END"	) )
	        break;
	    else
	    {
		std::string buf;

		buf = str_printf("Load_ship_file: bad section: %s.", word);
		bug( buf.c_str(), 0 );
		break;
	    }
	}
	FCLOSE( fp );
    }
    if ( !(found) )
      DISPOSE( ship );
    else
    {
       LINK( ship, first_ship, last_ship, next, prev );
       
       update_ship_modules( ship );
       
       ship->docking = SHIP_READY;
       if( !(ship->dockingports) )
         ship->dockingports = 0;

       for( bus = 0; bus < MAX_BUS; bus++ )
         if( ship->cockpit == serin[bus].cockpitvnum )
           isbus = TRUE;

       if ( ( !str_cmp("Trainer", ship->owner) || !str_cmp("Public",ship->owner) || ship->type == MOB_SHIP ) && !isbus )
       {

         if ( ship->shipclass != SHIP_PLATFORM && ship->type != MOB_SHIP && ship->shipclass != CAPITAL_SHIP )
         {
           extract_ship( ship );
           ship_to_room( ship , ship->shipyard );

           ship->location = ship->shipyard;
           ship->lastdoc = ship->shipyard;
           ship->shipstate = SHIP_LANDED;
           ship->docking = SHIP_READY;
           }

     if( ship->personalname == NULL )
       ship->personalname = STRALLOC(ship->name);

     ship->currspeed=0;
     ship->energy=ship->mod->maxenergy;
     ship->hull=ship->mod->maxhull;
     ship->shield=0;

     ship->statet0 = LASER_READY;
     ship->missilestate = LASER_READY;
     ship->statettractor = SHIP_READY;
     ship->statetdocking = SHIP_READY;
     ship->docking = SHIP_READY;

     ship->currjump=NULL;
     ship->target0=NULL;

     ship->hatchopen = FALSE;
     ship->bayopen = FALSE;

     ship->autorecharge = FALSE;
     ship->autotrack = FALSE;
     ship->autospeed = FALSE;


       }

       else if ( ship->cockpit == ROOM_SENATE_SHUTTLE ||
                 ship->cockpit == ROOM_CORUSCANT_TURBOCAR ||
                 ship->cockpit == ROOM_CORUSCANT_SHUTTLE ||
                 isbus  )
       {}
       else if ( ( pRoomIndex = get_room_index( ship->lastdoc ) ) != NULL
            && ship->shipclass != CAPITAL_SHIP && ship->shipclass != SHIP_PLATFORM && ship->type != MOB_SHIP )
       {
              LINK( ship, pRoomIndex->first_ship, pRoomIndex->last_ship, next_in_room, prev_in_room );
              ship->in_room = pRoomIndex;
              ship->location = ship->lastdoc;
       }


       if ( ship->shipclass == SHIP_PLATFORM || ship->type == MOB_SHIP || ship->shipclass == CAPITAL_SHIP )
       {
 
          ship_to_spaceobject(ship, spaceobject_from_name(ship->game, ship->home) );
/*  Do not need now that coordinates are saved in ship files.
          ship->vx = number_range( -5000 , 5000 );
          ship->vy = number_range( -5000 , 5000 );
          ship->vz = number_range( -5000 , 5000 );
 */ 
          ship->hx = 1;
          ship->hy = 1;
          ship->hz = 1;

    if( ship->vx == 0 && ship->vy == 0 && ship->vz == 0 )
     if ( ( ship->shipclass == SHIP_PLATFORM || ship->type == MOB_SHIP || ship->shipclass == CAPITAL_SHIP )
          && ship->home )
     {
          ship_to_spaceobject(ship, spaceobject_from_name(ship->game, ship->home) );
          ship->vx = number_range( -5000 , 5000 );
          ship->vy = number_range( -5000 , 5000 );
          ship->vz = number_range( -5000 , 5000 );
	  if( ship->spaceobject )
	  {
	    ship->vx += ship->spaceobject->xpos;
	    ship->vy += ship->spaceobject->ypos;
	    ship->vz += ship->spaceobject->zpos;
	  }
          ship->shipstate = SHIP_READY;
          ship->autopilot = TRUE;
          ship->autorecharge = TRUE;
          ship->shield = ship->maxshield;
     }
          
          ship->shipstate = SHIP_READY;
          ship->docking = SHIP_READY;
          ship->autopilot = TRUE;
          ship->autorecharge = TRUE;
          ship->shield = ship->maxshield;
       }

     if ( ship->type == MOB_SHIP )
     {
       ship->location = 0;
       ship->lastdoc = 0;
     }
       
     link_ship_rooms( ship->game, ship );

         if ( ship->type != MOB_SHIP && (clan = get_clan( ship->owner )) != NULL )
         {
          if ( ship->shipclass <= SHIP_PLATFORM )
             clan->spacecraft++;
          else
             clan->vehicles++;
         }
       ship->docking = SHIP_READY;

    }

    return found;
}

/*
 * Load in all the ship files.
 */
void load_ships( GameContext *game )
{
    FILE *fpList;
    std::string filename;
    std::string shiplist;
    std::string buf;
    SHIP_DATA *ship;
    ROOM_INDEX_DATA *pRoomIndex;


    first_ship	= NULL;
    last_ship	= NULL;
    first_missile = NULL;
    last_missile = NULL;

    log_string( "Loading ships..." );

    shiplist = str_printf("%s%s", SHIP_DIR, SHIP_LIST);
    FCLOSE( fpReserve );
    if ( ( fpList = fopen( shiplist.c_str(), "r" ) ) == NULL )
    {
	perror( shiplist.c_str() );
	exit( 1 );
    }

    for ( ; ; )
    {

	filename = feof( fpList ) ? "$" : fread_word( fpList );

	if ( filename[0] == '$' )
	  break;

	if ( !load_ship_file( game, filename ) )
	{
	  buf = str_printf("Cannot load ship file: %s", filename);
	  bug( buf.c_str(), 0 );
	}

    }
    FCLOSE( fpList );

    for( ship = first_ship; ship; ship = ship->next )
      if( !ship->location )
        if ( ( pRoomIndex = get_room_index( ship->lastdoc ) ) != NULL
            && ship->shipclass != CAPITAL_SHIP && ship->shipclass != SHIP_PLATFORM )
        {

              LINK( ship, pRoomIndex->first_ship, pRoomIndex->last_ship, next_in_room, prev_in_room );
              ship->in_room = pRoomIndex;
              ship->location = ship->lastdoc;
        }


    log_string(" Done ships " );
    fpReserve = fopen( NULL_FILE, "r" );
    return;
}

void resetship( SHIP_DATA *ship )
{
     ship->shipstate = SHIP_READY;
     ship->docking = SHIP_READY;
     ship->docked = NULL;
     if ( (ship->shipclass != SHIP_PLATFORM && ship->shipclass != CAPITAL_SHIP) && ship->type != MOB_SHIP )
     {
           extract_ship( ship );
           ship_to_room( ship , ship->shipyard ); 
     
           ship->location = ship->shipyard;
           ship->lastdoc = ship->shipyard; 
           ship->shipstate = SHIP_LANDED;
     }
     
     if (ship->spaceobject)
        ship_from_spaceobject( ship, ship->spaceobject );

     ship->currspeed=0;
     ship->energy=ship->mod->maxenergy;
     ship->hull=ship->mod->maxhull;
     ship->shield=0;

     ship->statet0 = LASER_READY;
     ship->missilestate = LASER_READY;

     ship->currjump=NULL;
     ship->target0=NULL;

     ship->hatchopen = FALSE;
     ship->bayopen = FALSE;

     ship->autorecharge = FALSE;
     ship->autotrack = FALSE;
     ship->autospeed = FALSE;

#ifndef NODEATH
#ifndef NODEATHSHIP
     if ( str_cmp("Trainer", ship->owner) && str_cmp("Public",ship->owner) && ship->type != MOB_SHIP )
     {
        CLAN_DATA *clan;

        if ( ship->type != MOB_SHIP && (clan = get_clan( ship->owner )) != NULL )
        {
          if ( ship->shipclass <= SHIP_PLATFORM )
             clan->spacecraft--;
          else
             clan->vehicles--;
        }

        STRFREE( ship->owner );
        ship->owner = STRALLOC( "" );
        STRFREE( ship->pilot );
        ship->pilot = STRALLOC( "" );
        STRFREE( ship->copilot );
        ship->copilot = STRALLOC( "" );
     }
#endif
#endif
    if (!(ship->home))
    {
     if ( ship->type == SHIP_REBEL || ( ship->type == MOB_SHIP && ((!str_cmp( ship->owner , "The Rebel Alliance" )) || (!str_cmp( ship->owner , "The New Republic" )))))
     {
         STRFREE( ship->home );
         ship->home = STRALLOC( "Mon Calamari" );
     }
     else if ( ship->type == SHIP_IMPERIAL || ( ship->type == MOB_SHIP && !str_cmp(ship->owner, "the empire") ))
     {
          STRFREE( ship->home );
          ship->home = STRALLOC( "coruscant" );
     }
     else if ( ship->type == SHIP_CIVILIAN)
     {
          STRFREE( ship->home );
          ship->home = STRALLOC( "corperate" );
     }
   }

     save_ship(ship);
}

void do_resetship( CHAR_DATA *ch, char *argument )
{    
     SHIP_DATA *ship;
     
     ship = get_ship( ch->game, argument );
     if (ship == NULL)
     {
        send_to_char("&RNo such ship!",ch);
        return;
     }
     
     resetship( ship ); 
     
     if ( ( ship->shipclass == SHIP_PLATFORM || ship->type == MOB_SHIP || ship->shipclass == CAPITAL_SHIP )
          && ship->home )
     {
          ship_to_spaceobject(ship, spaceobject_from_name(ship->game, ship->home) );
          ship->vx = number_range( -5000 , 5000 );
          ship->vy = number_range( -5000 , 5000 );
          ship->vz = number_range( -5000 , 5000 );
	  if( ship->spaceobject )
	  {
	    ship->vx += ship->spaceobject->xpos;
	    ship->vy += ship->spaceobject->ypos;
	    ship->vz += ship->spaceobject->zpos;
	  }
          ship->shipstate = SHIP_READY;
          ship->autopilot = TRUE;
          ship->autorecharge = TRUE;
          ship->shield = ship->mod->maxshield;
     }

}

void do_setship( CHAR_DATA *ch, char *argument )
{
    std::string arg1;
    std::string arg2;
    std::string argstr = argument;
    SHIP_DATA *ship;
    int  tempnum;
    ROOM_INDEX_DATA *roomindex;
    
    if ( IS_NPC( ch ) )
    {
        send_to_char( "Huh?\n", ch );
        return;
    }

    argstr = one_argument( argstr, arg1 );
    argstr = one_argument( argstr, arg2 );

    if ( arg1.empty() || arg2.empty() || argstr.empty() )
    {
        send_to_char( "Usage: setship <ship> <field> <values>\n", ch );
        send_to_char( "\nField being one of:\n", ch );
        send_to_char( "filename name personalname owner copilot pilot description home\n", ch );
        send_to_char( "cockpit entrance hanger turret cargohold\n", ch );
        send_to_char( "engineroom firstroom lastroom shipyard\n", ch );
        send_to_char( "manuever speed hyperspeed tractorbeam\n", ch );
        send_to_char( "lasers missiles shield hull energy chaff\n", ch );
        send_to_char( "comm sensor astroarray class torpedos\n", ch );
        send_to_char( "pilotseat coseat gunseat navseat rockets alarm\n", ch );
        send_to_char( "ions maxcargo cargo# dockingports guard (0-1)\n", ch );
        send_to_char( "\n\n", ch );
        send_to_char( "Cargo types:\n", ch );
        send_to_char( "Numbered 1 - 8\n", ch );
        send_to_char( "food metal equipment lumber medical spice\n", ch );
        send_to_char( "weapons valuables\n", ch );



        return;
    }

    ship = get_ship( ch->game, arg1 );
    if ( !ship )
    {
        send_to_char( "No such ship.\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "owner" ) )
    {
         CLAN_DATA *clan;
         if ( ship->type != MOB_SHIP && (clan = get_clan( ship->owner )) != NULL )
         {
          if ( ship->shipclass <= SHIP_PLATFORM )
             clan->spacecraft--;
          else
             clan->vehicles--;
         }
        STRFREE( ship->owner );
        ship->owner = STRALLOC( argstr );
        send_to_char( "Done.\n", ch );
        save_ship( ship );
        if ( ship->type != MOB_SHIP && (clan = get_clan( ship->owner )) != NULL )
         {
          if ( ship->shipclass <= SHIP_PLATFORM )
             clan->spacecraft++;
          else
             clan->vehicles++;
         }
	return;
    }
    
    if ( !str_cmp( arg2, "home" ) )
    {
	STRFREE( ship->home );
	ship->home = STRALLOC( argstr );
	send_to_char( "Done.\n", ch );
	save_ship( ship );
	return;
    }

    if ( !str_cmp( arg2, "pilot" ) )
    {
	STRFREE( ship->pilot );
	ship->pilot = STRALLOC( argstr );
	send_to_char( "Done.\n", ch );
	save_ship( ship );
	return;
    }

    if ( !str_cmp( arg2, "copilot" ) )
    {
	STRFREE( ship->copilot );
	ship->copilot = STRALLOC( argstr );
	send_to_char( "Done.\n", ch );
	save_ship( ship );
	return;
    }

    if ( !str_cmp( arg2, "firstroom" ) )
    {   
        tempnum = strtoi( argstr ); 
    	roomindex = get_room_index(tempnum);
    	if (roomindex == NULL)
    	{
    	   send_to_char("That room doesn't exist.\n",ch);
    	   return;
    	} 
	    ship->set_firstroom(tempnum);
        ship->set_lastroom(tempnum);
        ship->cockpit = tempnum;
        ship->coseat = tempnum;
        ship->pilotseat = tempnum;
        ship->gunseat = tempnum;
        ship->navseat = tempnum;
        ship->entrance = tempnum;
        ship->hanger = 0;
	send_to_char( "You will now need to set the other rooms in the ship.\n", ch );
	save_ship( ship );
	return;
    }

    if ( !str_cmp( arg2, "lastroom" ) )
    {   
        tempnum = strtoi( argstr ); 
    	roomindex = get_room_index(tempnum);
    	if (roomindex == NULL)
    	{
    	   send_to_char("That room doesn't exist.\n",ch);
    	   return;
    	} 
    	if ( tempnum < ship->get_firstroom() )
    	{
    	   send_to_char("The last room on a ship must be greater than or equal to the first room.\n",ch);
           return;
    	}
    	if ( ship->shipclass == FIGHTER_SHIP && (tempnum - ship->get_firstroom()) > 5 )
    	{
    	   send_to_char("Starfighters may have up to 5 rooms only.\n",ch);
    	   return;
    	}  
	if ( ship->shipclass == MIDSIZE_SHIP && (tempnum - ship->get_firstroom()) > 25 )
    	{
    	   send_to_char("Midships may have up to 25 rooms only.\n",ch);
    	   return;
    	}  
	if ( ship->shipclass == CAPITAL_SHIP && (tempnum - ship->get_firstroom()) > 100 )
    	{
    	   send_to_char("Capital Ships may have up to 100 rooms only.\n",ch);
    	   return;
    	}
	ship->set_lastroom(tempnum);
	send_to_char( "Done.\n", ch );
	save_ship( ship );
	return;
    }

    if ( !str_cmp( arg2, "cockpit" ) )
    {   
        tempnum = strtoi( argstr ); 
    	roomindex = get_room_index(tempnum);
    	if (roomindex == NULL)
    	{
    	   send_to_char("That room doesn't exist.\n",ch);
    	   return;
    	} 
    	if ( tempnum < ship->get_firstroom() || tempnum > ship->get_lastroom() )
    	{
    	   send_to_char("That room number is not in that ship .. \nIt must be between Firstroom and Lastroom.\n",ch);
           return;
    	}
	if ( tempnum == ship->hanger )
    	{
    	   send_to_char("That room is already being used by another part of the ship\n",ch);
           return;
    	}
	ship->cockpit = tempnum;
	send_to_char( "Done.\n", ch );
	save_ship( ship );
	return;
    }
    
    if ( !str_cmp( arg2, "pilotseat" ) )
    {   
        tempnum = strtoi( argstr ); 
    	roomindex = get_room_index(tempnum);
    	if (roomindex == NULL)
    	{
    	   send_to_char("That room doesn't exist.\n",ch);
    	   return;
    	}
    	if ( tempnum < ship->get_firstroom() || tempnum > ship->get_lastroom() )
    	{
    	   send_to_char("That room number is not in that ship .. \nIt must be between Firstroom and Lastroom.\n",ch);
           return;
    	}
	if ( tempnum == ship->hanger )
    	{
    	   send_to_char("That room is already being used by another part of the ship\n",ch);
           return;
    	}
	ship->pilotseat = tempnum;
	send_to_char( "Done.\n", ch );
	save_ship( ship );
	return;
    }
    if ( !str_cmp( arg2, "coseat" ) )
    {   
        tempnum = strtoi( argstr );
    	roomindex = get_room_index(tempnum);
    	if (roomindex == NULL)
    	{
    	   send_to_char("That room doesn't exist.\n",ch);
    	   return;
    	} 
    	if ( tempnum < ship->get_firstroom() || tempnum > ship->get_lastroom() )
    	{
    	   send_to_char("That room number is not in that ship .. \nIt must be between Firstroom and Lastroom.\n",ch);
           return;
    	}
	if ( tempnum == ship->hanger )
    	{
    	   send_to_char("That room is already being used by another part of the ship\n",ch);
           return;
    	}
	ship->coseat = tempnum;
	send_to_char( "Done.\n", ch );
	save_ship( ship );
	return;
    }
    if ( !str_cmp( arg2, "navseat" ) )
    {   
        tempnum = strtoi( argstr ); 
    	roomindex = get_room_index(tempnum);
    	if (roomindex == NULL)
    	{
    	   send_to_char("That room doesn't exist.\n",ch);
    	   return;
    	} 
    	if ( tempnum < ship->get_firstroom() || tempnum > ship->get_lastroom() )
    	{
    	   send_to_char("That room number is not in that ship .. \nIt must be between Firstroom and Lastroom.\n",ch);
           return;
    	}
	if ( tempnum == ship->hanger )
    	{
    	   send_to_char("That room is already being used by another part of the ship\n",ch);
           return;
    	}
	ship->navseat = tempnum;
	send_to_char( "Done.\n", ch );
	save_ship( ship );
	return;
    }
    if ( !str_cmp( arg2, "gunseat" ) )
    {   
        tempnum = strtoi( argstr ); 
    	roomindex = get_room_index(tempnum);
    	if (roomindex == NULL)
    	{
    	   send_to_char("That room doesn't exist.\n",ch);
    	   return;
    	}
    	if ( tempnum < ship->get_firstroom() || tempnum > ship->get_lastroom() )
    	{
    	   send_to_char("That room number is not in that ship .. \nIt must be between Firstroom and Lastroom.\n",ch);
           return;
    	}
	if ( tempnum == ship->hanger )
    	{
    	   send_to_char("That room is already being used by another part of the ship\n",ch);
           return;
    	}
	ship->gunseat = tempnum;
	send_to_char( "Done.\n", ch );
	save_ship( ship );
	return;
    }
    
    if ( !str_cmp( arg2, "entrance" ) )
    {
        tempnum = strtoi( argstr );
    	roomindex = get_room_index(tempnum);
    	if (roomindex == NULL)
    	{
    	   send_to_char("That room doesn't exist.\n",ch);
    	   return;
    	} 
    	if ( tempnum < ship->get_firstroom() || tempnum > ship->get_lastroom() )
    	{
    	   send_to_char("That room number is not in that ship .. \nIt must be between Firstroom and Lastroom.\n",ch);
           return;
    	}
	ship->entrance = tempnum;
	send_to_char( "Done.\n", ch );
	save_ship( ship );
	return;
    }

    if ( !str_cmp( arg2, "turret" ) )
    {
    	std::string arg3;
    	std::string arg4;
    	std::string arg5;
    	TURRET_DATA *turret;
    	int turretvnum, typeassigned;
        argstr = one_argument( argstr, arg3 );
        argstr = one_argument( argstr, arg4 );
        argstr = one_argument( argstr, arg5 );
        
        if ( arg3.empty() || arg4.empty() )
    	{
    	   send_to_char("Syntax: setship <ship> turret <vnum> <field> <value>\n",ch);
    	   send_to_char("Fields are: create delete type\n",ch);
           return;
    	}
    	
    	if ( !is_number(arg3) )
    	{
    	   send_to_char("Vnum must be the room that the turret is or will be located in.\n",ch);
           return;
    	}
    	
    	turretvnum = strtoi(arg3);
    	
    	if ( !str_cmp( arg4, "create" ) )
    	{
//    	   send_to_char("Not quite done with this yet-DV.\n",ch);
//           return;
//    	}
	   CREATE( turret, TURRET_DATA, 1 );
	   ship->shipclass == MIDSIZE_SHIP ? (turret->type = QUAD_TURRET) : (turret->type = TURBOLASER_TURRET);
	   turret->roomvnum	= turretvnum;
	   turret->state	= LASER_READY;

	   LINK( turret, ship->first_turret, ship->last_turret, next, prev );
	   save_ship(ship);
	   return;
	}
    	if ( !str_cmp( arg4, "delete" ) )
    	{
//    	   send_to_char("Not quite done with this yet-DV.\n",ch);
//	   return;
//    	}
    	   if( !ship->first_turret )
    	   {
    	     send_to_char("That ship has no turrets.\n",ch);
             return;
    	   }
    	   for ( turret = ship->first_turret; turret; turret = turret->next )
    	   {
    	   	if ( turret->roomvnum == turretvnum )
    	   	  break;
    	   }
    	   if ( !turret )
    	   {
    	     send_to_char("There is no turret with that vnum on that ship.\n",ch);
             return;
    	   }
	   turret->type	 	= 0;
	   turret->roomvnum	= 0;
	   turret->state	= 0;

	   UNLINK( turret, ship->first_turret, ship->last_turret, next, prev );
	   DISPOSE(turret);
	   save_ship(ship);
	   return;
	}

    	if ( !str_cmp( arg4, "type" ) )
    	{
    	   if( arg5.empty() || !is_number(arg5) )
    	   {
    	     send_to_char("Syntax: setship <ship> turret <vnum> type <type number>.\n",ch);
             return;
    	   }
    	   typeassigned = strtoi(arg5);
    	   if ( typeassigned > MAX_TURRET_TYPE )
    	   {
    	     int x;
    	     send_to_char("Turret type invalid. Valid types follow.\n",ch);
    	     for ( x = 0; x < MAX_TURRET_TYPE; x++ )
    	     {
    	       send_to_char( turretstats[x].name, ch );
    	       send_to_char( "\n", ch );
    	     }
    	     return;
    	   }
    	   if( !ship->first_turret )
    	   {
    	     send_to_char("That ship has no turrets.\n",ch);
             return;
    	   }
    	   for ( turret = ship->first_turret; turret; turret = turret->next )
    	   {
    	   	if ( turret->roomvnum == turretvnum )
    	   	  break;
    	   }
    	   if ( !turret )
    	   {
    	     send_to_char("There is no turret with that vnum on that ship.\n",ch);
             return;
    	   }
    	   turret->type = typeassigned;
    	   send_to_char("Done.\n", ch);
    	   save_ship(ship);
    	   return;
    	   
	}          
    }


/*  if ( !str_cmp( arg2, "turret1" ) )
    {   
        tempnum = atoi(argument); 
    	roomindex = get_room_index(tempnum);
    	if (roomindex == NULL)
    	{
    	   send_to_char("That room doesn't exist.\n",ch);
    	   return;
    	} 
	if ( tempnum < ship->get_firstroom() || tempnum > ship->get_lastroom() )
    	{
    	   send_to_char("That room number is not in that ship .. \nIt must be between Firstroom and Lastroom.\n",ch);
           return;
    	}
    	if ( ship->shipclass == FIGHTER_SHIP )
    	{
    	   send_to_char("Starfighters can't have extra laser turrets.\n",ch);
    	   return;
    	}
	if ( tempnum == ship->cockpit || tempnum == ship->entrance ||
    	     tempnum == ship->turret2 || tempnum == ship->hanger || tempnum == ship->engineroom )
    	{
    	   send_to_char("That room is already being used by another part of the ship\n",ch);
           return;
    	}
	ship->turret1 = tempnum;
	send_to_char( "Done.\n", ch );
	save_ship( ship );
	return;
    }

    if ( !str_cmp( arg2, "turret2" ) )
    {   
        tempnum = atoi(argument); 
    	roomindex = get_room_index(tempnum);
    	if (roomindex == NULL)
    	{
    	   send_to_char("That room doesn't exist.\n",ch);
    	   return;
    	} 
	if ( tempnum < ship->get_firstroom() || tempnum > ship->get_lastroom() )
    	{
    	   send_to_char("That room number is not in that ship .. \nIt must be between Firstroom and Lastroom.\n",ch);
           return;
    	}
	if ( ship->shipclass == FIGHTER_SHIP )
    	{
    	   send_to_char("Starfighters can't have extra laser turrets.\n",ch);
    	   return;
    	}
	if ( tempnum == ship->cockpit || tempnum == ship->entrance ||
    	     tempnum == ship->turret1 || tempnum == ship->hanger || tempnum == ship->engineroom )
    	{
    	   send_to_char("That room is already being used by another part of the ship\n",ch);
           return;
    	}
	ship->turret2 = tempnum;
	send_to_char( "Done.\n", ch );
	save_ship( ship );
	return;
    }
*/
    if ( !str_cmp( arg2, "hangar" ) || !str_cmp( arg2, "hanger" ) )
    {   
        tempnum = strtoi(argstr);
    	roomindex = get_room_index(tempnum);
    	if (roomindex == NULL && strtoi(argstr) != 0 )
    	{
    	   send_to_char("That room doesn't exist.\n",ch);
    	   return;
    	} 
	if (( tempnum < ship->get_firstroom() || tempnum > ship->get_lastroom() ) && (strtoi(argstr) != 0 ))
    	{
    	   send_to_char("That room number is not in that ship .. \nIt must be between Firstroom and Lastroom.\n",ch);
           return;
    	}
    	if ( tempnum == ship->cockpit || tempnum == ship->entrance || tempnum == ship->engineroom )
    	{
    	   send_to_char("That room is already being used by another part of the ship\n",ch);
           return;
    	}
	if ( ship->shipclass == FIGHTER_SHIP )
	{
	   send_to_char("Starfighters are to small to have hangers for other ships!\n",ch);
	   return;
	}
	ship->hanger = tempnum;
	send_to_char( "Done.\n", ch );
	save_ship( ship );
	return;
    }
/*
    if ( !str_cmp( arg2, "cargohold" ) )
    {   
        tempnum = atoi(argument); 
    	roomindex = get_room_index(tempnum);
    	if (roomindex == NULL)
    	{
    	   send_to_char("That room doesn't exist.\n",ch);
    	   return;
    	} 
	if ( tempnum < ship->get_firstroom() || tempnum > ship->get_lastroom() )
    	{
    	   send_to_char("That room number is not in that ship .. \nIt must be between Firstroom and Lastroom.\n",ch);
           return;
    	}
    	if ( tempnum == ship->cockpit || tempnum == ship->entrance || tempnum == ship->engineroom )
    	{
    	   send_to_char("That room is already being used by another part of the ship\n",ch);
           return;
    	}
	if ( ship->shipclass == FIGHTER_SHIP )
	{
	   send_to_char("Starfighters are to small to have cargo holds!\n",ch);
	   return;
	}
	ship->cargohold = tempnum;
	send_to_char( "Done.\n", ch );
	save_ship( ship );
	return;
    }
*/
   if ( !str_cmp( arg2, "engineroom" ) )
    {   
        tempnum = strtoi( argstr ); 
    	roomindex = get_room_index(tempnum);
    	if (roomindex == NULL)
    	{
    	   send_to_char("That room doesn't exist.\n",ch);
    	   return;
    	} 
	if ( tempnum < ship->get_firstroom() || tempnum > ship->get_lastroom() )
    	{
    	   send_to_char("That room number is not in that ship .. \nIt must be between Firstroom and Lastroom.\n",ch);
           return;
    	}
    	if ( tempnum == ship->cockpit || tempnum == ship->entrance || tempnum == ship->hanger )
    	{
    	   send_to_char("That room is already being used by another part of the ship\n",ch);
           return;
    	}
	ship->engineroom = tempnum;
	send_to_char( "Done.\n", ch );
	save_ship( ship );
	return;
    }

    if ( !str_cmp( arg2, "shipyard" ) )
    {
        tempnum = strtoi( argstr );
    	roomindex = get_room_index(tempnum);
    	if (roomindex == NULL)
    	{
    	   send_to_char("That room doesn't exist.",ch);
    	   return;
    	} 
	ship->shipyard = tempnum;
	send_to_char( "Done.\n", ch );
	save_ship( ship );
	return;
    }

    if ( !str_cmp( arg2, "type" ) )
    {
	if ( !str_cmp( argstr, "rebel" ) )
	  ship->type = SHIP_REBEL;
	else
	if ( !str_cmp( argstr, "imperial" ) )
	  ship->type = SHIP_IMPERIAL;
	else
	if ( !str_cmp( argstr, "civilian" ) )
	  ship->type = SHIP_CIVILIAN;
	else
	if ( !str_cmp( argstr, "mob" ) )
	  ship->type = MOB_SHIP;
	else
	{
	   send_to_char( "Ship type must be either: rebel, imperial, civilian or mob.\n", ch );
	   return;
	}
	send_to_char( "Done.\n", ch );
	save_ship( ship );
	return;
    }

    if ( !str_cmp( arg2, "name" ) )
    {
	STRFREE( ship->name );
	ship->name = STRALLOC( argstr );
	send_to_char( "Done.\n", ch );
	save_ship( ship );
	return;
    }

    if ( !str_cmp( arg2, "personalname" ) )
    {
      if ( ship->personalname )
	STRFREE( ship->personalname );
      ship->personalname = STRALLOC( argstr );
      send_to_char( "Done.\n", ch );
      save_ship( ship ); 
      return;
    }

    if ( !str_cmp( arg2, "filename" ) )
    {
	STR_DISPOSE( ship->filename );
	ship->filename = str_dup( argstr );
	send_to_char( "Done.\n", ch );
	save_ship( ship );
	write_ship_list( ship->game );
	return;
    }

    if ( !str_cmp( arg2, "desc" ) )
    {
	STRFREE( ship->description );
	ship->description = STRALLOC( argstr );
	send_to_char( "Done.\n", ch );
	save_ship( ship );
	return;
    }

    if ( !str_cmp( arg2, "dockingports" ) )
    {   
	ship->dockingports = URANGE( -1, strtoi(argstr) , 20 );
	send_to_char( "Done.\n", ch );
	save_ship( ship );
	return;
    }

    if ( !str_cmp( arg2, "guard" ) )
    {   
	ship->guard = URANGE( -1, strtoi(argstr) , 1 );
	send_to_char( "Done.\n", ch );
	save_ship( ship );
	return;
    }

    if ( !str_cmp( arg2, "manuever" ) )
    {   
	ship->manuever = URANGE( 0, strtoi(argstr) , 250 );
	send_to_char( "Done.\n", ch );
	save_ship( ship );
	return;
    }

    if ( !str_cmp( arg2, "upgradeblock" ) )
    {   
        if ( ch->top_level < LEVEL_SUPREME )
          return;
	ship->upgradeblock = strtoi(argstr);
	send_to_char( "Done.\n", ch );
	save_ship( ship );
	return;
    }

    if ( !str_cmp( arg2, "lasers" ) )
    {
   if ( ch->top_level == LEVEL_SUPREME )
     ship->lasers = URANGE( 0, strtoi(argstr) , 20 );
   else
	  ship->lasers = URANGE( 0, strtoi(argstr) , 10 );
	send_to_char( "Done.\n", ch );
	save_ship( ship );
	return;
    }

    if ( !str_cmp( arg2, "ions" ) )
    {
   if ( ch->top_level == LEVEL_SUPREME )
     ship->ions = URANGE( 0, strtoi(argstr) , 20 );
   else
	  ship->ions = URANGE( 0, strtoi(argstr) , 10 );
	send_to_char( "Done.\n", ch );
	save_ship( ship );
	return;
    }

    if ( !str_cmp( arg2, "gravitypower" ) )
    {
        ship->mod->gravitypower = strtoi(argstr);
      save_ship( ship );
      return;
    }

    if ( !str_cmp( arg2, "gravproj" ) )
    {
        ship->mod->gravproj= strtoi(argstr);
      save_ship( ship );
      return;
    }

    if ( !str_cmp( arg2, "maxcargo" ) )
    {
   if ( ch->top_level == LEVEL_SUPREME )
     ship->maxcargo = URANGE( 0, strtoi(argstr) , 30000 );
   else
	  ship->maxcargo = URANGE( 0, strtoi(argstr) , 1000 );
	send_to_char( "Done.\n", ch );
	save_ship( ship );
	return;
    }

/*  if ( !str_cmp( arg2, "cargo1" ) )
    {
   if ( ch->top_level == LEVEL_SUPREME )
     ship->cargo1 = URANGE( 0, strtoi(argstr) , 30000 );
   else
	  ship->cargo1 = URANGE( 0, strtoi(argstr) , ship->maxcargo );
	send_to_char( "Done.\n", ch );
	save_ship( ship );
	return;
    }
*/

    if ( !str_cmp( arg2, "class" ) )
    {
	ship->shipclass = URANGE( 0, strtoi(argstr) , 9 );
	send_to_char( "Done.\n", ch );
	save_ship( ship );
	return;
    }

    if ( !str_cmp( arg2, "missiles" ) )
    {
	ship->missiles = URANGE( 0, strtoi(argstr) , 255 );
	send_to_char( "Done.\n", ch );
	save_ship( ship );
	return;
    }

    if ( !str_cmp( arg2, "torpedos" ) )
    {
	ship->torpedos = URANGE( 0, strtoi(argstr) , 255 );
	send_to_char( "Done.\n", ch );
	save_ship( ship );
	return;
    }

    if ( !str_cmp( arg2, "rockets" ) )
    {
	ship->rockets = URANGE( 0, strtoi(argstr) , 255 );
	send_to_char( "Done.\n", ch );
	save_ship( ship );
	return;
    }

    if ( !str_cmp( arg2, "speed" ) )
    {
   if ( ch->top_level == LEVEL_SUPREME )
     ship->realspeed = URANGE( 0, strtoi(argstr) , 255 );
   else
	  ship->realspeed = URANGE( 0, strtoi(argstr) , 150 );
	send_to_char( "Done.\n", ch );
	save_ship( ship );
	return;
    }

    if ( !str_cmp( arg2, "tractorbeam" ) )
    {
	ship->tractorbeam = URANGE( 0, strtoi(argstr) , 255 );
	send_to_char( "Done.\n", ch );
	save_ship( ship );
	return;
    }
 
    if ( !str_cmp( arg2, "hyperspeed" ) )
    {
	ship->hyperspeed = URANGE( 0, strtoi(argstr) , 255 );
	send_to_char( "Done.\n", ch );
	save_ship( ship );
	return;
    }

    if ( !str_cmp( arg2, "shield" ) )
    {
   if ( ch->top_level == LEVEL_SUPREME )
     ship->maxshield = URANGE( 0, strtoi(argstr) , 30000 );
   else
	  ship->maxshield = URANGE( 0, strtoi(argstr) , 1000 );
	send_to_char( "Done.\n", ch );
	save_ship( ship );
	return;
    }

    if ( !str_cmp( arg2, "hull" ) )
    {
   if ( ch->top_level == LEVEL_SUPREME )
	{ ship->hull = URANGE( 1, strtoi(argstr) , 30000 );
     ship->maxhull = URANGE( 1, strtoi(argstr) , 30000 );
   }
   else {
	  ship->hull = URANGE( 1, strtoi(argstr) , 20000 );
	  ship->maxhull = URANGE( 1, strtoi(argstr) , 20000 );
   }
	send_to_char( "Done.\n", ch );
	save_ship( ship );
	return;
    }

    if ( !str_cmp( arg2, "energy" ) )
    {
	ship->energy = URANGE( 0, strtoi(argstr) , 30000 );
	ship->maxenergy = URANGE( 0, strtoi(argstr) , 30000 );
	send_to_char( "Done.\n", ch );
	save_ship( ship );
	return;
    }

    if ( !str_cmp( arg2, "sensor" ) )
    {
	ship->sensor = URANGE( 0, strtoi(argstr) , 255 );
	send_to_char( "Done.\n", ch );
	save_ship( ship );
	return;
    }

    if ( !str_cmp( arg2, "astroarray" ) )
    {
	ship->astro_array = URANGE( 0, strtoi(argstr) , 255 );
	send_to_char( "Done.\n", ch );
	save_ship( ship );
	return;
    }

    if ( !str_cmp( arg2, "comm" ) )
    {
	ship->comm = URANGE( 0, strtoi(argstr) , 255 );
	send_to_char( "Done.\n", ch );
	save_ship( ship );
	return;
    }

    if ( !str_cmp( arg2, "chaff" ) )
    {
   if ( ch->top_level == LEVEL_SUPREME )
   { ship->chaff = URANGE( 0, strtoi(argstr) , 255 );
   }
   else {
	  ship->chaff = URANGE( 0, strtoi(argstr) , 25 );
   }
	send_to_char( "Done.\n", ch );
	save_ship( ship );
	return;
    }

    if ( !str_cmp(arg2,"alarm") )
    {
        ship->alarm = URANGE(0,strtoi(argstr),5);
        send_to_char("Done.\n",ch);
        save_ship(ship);
        return;
    }

    if ( !str_cmp( arg2, "maxintmodules" ) )
	{
		ship->maxintmodules = strtoi(argstr);
		return;
	}
    if ( !str_cmp( arg2, "maxextmodules" ) )
	{
		ship->maxextmodules = strtoi(argstr);
		return;
	}

    do_setship( ch, "" );
    return;
}

void do_showship( CHAR_DATA *ch, char *argument )
{
    SHIP_DATA *ship;
    TURRET_DATA *turret;
    sh_int turretnum = 0;

    if ( IS_NPC( ch ) )
    {
	send_to_char( "Huh?\n", ch );
	return;
    }

    if ( argument[0] == '\0' )
    {
	send_to_char( "Usage: showship <ship>\n", ch );
	return;
    }

    ship = get_ship( ch->game, argument );
    if ( !ship )
    {
	send_to_char( "No such ship.\n", ch );
	return;
    }
    set_char_color( AT_YELLOW, ch );
    ch_printf( ch, "%s %s : %s (%s)\nFilename: %s ShipID: %ld\n",
			ship->type == SHIP_REBEL ? "Rebel Alliance" :
		       (ship->type == SHIP_IMPERIAL ? "Imperial" : 
		       (ship->type == SHIP_CIVILIAN ? "Civilian" : "Mob" ) ),
		        ship->shipclass == FIGHTER_SHIP ? "Starfighter" :
		       (ship->shipclass == MIDSIZE_SHIP ? "Midship" : 
		       (ship->shipclass == CAPITAL_SHIP ? "Capital Ship" : 
		       (ship->shipclass == SHIP_PLATFORM ? "Platform" : 
		       (ship->shipclass == CLOUD_CAR ? "Cloudcar" : 
		       (ship->shipclass == OCEAN_SHIP ? "Boat" :
		       (ship->shipclass == LAND_SPEEDER ? "Speeder" :
		       (ship->shipclass == WHEELED ? "Wheeled Transport" : 
		       (ship->shipclass == LAND_CRAWLER ? "Crawler" : 
		       (ship->shipclass == WALKER ? "Walker" : "Unknown" ) ) ) ) ) ) ) ) ), 
    			ship->name, 
    			ship->personalname,
    			ship->filename,
    			ship->shipID);
    ch_printf( ch, "Home: %s   Description: %s\nOwner: %s   Pilot: %s   Copilot: %s\n",
    			ship->home,  ship->description,
    			ship->owner, ship->pilot,  ship->copilot );
    ch_printf( ch, "Current Jump Destination: %s  Jump Point: %s\n", (ship->currjump ? ship->currjump->name : "(null)"), (ship->lastsystem ? ship->lastsystem->name : "(null)" ));
    ch_printf( ch, "Firstroom: %d   Lastroom: %d",
    			ship->get_firstroom(),
    			ship->get_lastroom());
    ch_printf( ch, "Cockpit: %d   Entrance: %d   Hanger: %d  Engineroom: %d\n",
    			ship->cockpit,
    			ship->entrance,
    			ship->hanger,
    			ship->engineroom);
    ch_printf( ch, "Pilotseat: %d   Coseat: %d   Navseat: %d  Gunseat: %d\n",
    			ship->pilotseat,
    			ship->coseat,
    			ship->navseat,
    			ship->gunseat);
    ch_printf( ch, "Location: %d   Lastdoc: %d   Shipyard: %d\n",
    			ship->location,
    			ship->lastdoc,
    			ship->shipyard);
    ch_printf( ch, "Tractor Beam: %d   Comm: %d   Sensor: %d   Astro Array: %d\n",
    			ship->mod->tractorbeam,
    			ship->mod->comm,
    			ship->mod->sensor,
    			ship->mod->astro_array);
    ch_printf( ch, "Lasers: %d  Ions: %d   Laser Condition: %s\n",
    			ship->mod->lasers, ship->mod->ions,
    			ship->statet0 == LASER_DAMAGED ? "Damaged" : "Good");

   if( ship->first_turret )
     for( turret = ship->first_turret, turretnum = 1; turret; turret = turret->next, turretnum++ )
       ch_printf( ch, "Turret%d Type: %d Vnum: %d State: %s\n",
         turretnum,
         turret->type, 
         turret->roomvnum,
         turret->state == LASER_DAMAGED ? "Damaged" : "Good");

    ch_printf( ch, "Missiles: %d  Torpedos: %d  Rockets: %d  Launchers: %dCondition: %s\n",
       			ship->missiles,
    			ship->torpedos,
    			ship->rockets,
    			ship->mod->launchers,
    			ship->missilestate == MISSILE_DAMAGED ? "Damaged" : "Good");		
    ch_printf( ch, "Hull: %d/%d  Ship Condition: %s\n",
                        ship->hull,
    		        ship->mod->maxhull,
    			ship->shipstate == SHIP_DISABLED ? "Disabled" : "Running");
    		
    ch_printf( ch, "Shields: %d/%d   Energy(fuel): %d/%d   Chaff: %d Launchers: %d\n",
                        ship->shield,
    		        ship->mod->maxshield,
    		        ship->energy,
    		        ship->mod->maxenergy,
    		        ship->chaff,
    		        ship->mod->defenselaunchers);
    if( ship->mod )
      ch_printf (ch, "Gravity Projector: %d/%d\n",
        ship->mod->gravproj,
        ship->mod->gravitypower );
    else
      ch_printf (ch, "Gravity Projector: (NULL)\n");
        		        
    ch_printf( ch, "Current Coordinates: %.0f %.0f %.0f\n",
                        ship->vx, ship->vy, ship->vz );
    ch_printf( ch, "Current Heading: %.0f %.0f %.0f\n",
                        ship->hx, ship->hy, ship->hz );
    ch_printf( ch, "Speed: %d/%d   Hyperspeed: %d   Manueverability: %d\n",
                        ship->currspeed, ship->mod->realspeed, ship->mod->hyperspeed , ship->mod->manuever );                    
    ch_printf( ch, "Docked: ");
    space_validate_ship_links( ch, ship, false );
    if ((ship->docked) != NULL)
	{
	    ch_printf( ch, "with %s",ship->docked->name);
	}
	else
	{
	    ch_printf( ch, "NO");
	}
	ch_printf(ch, "  Docking Ports: %d", ship->dockingports );
    ch_printf(ch,"  Alarm: %d   ",ship->alarm);
    
    if(ship->upgradeblock)
        ch_printf( ch, "UpgradeBlock: %d\n", ship->upgradeblock );
    else
        ch_printf( ch, "UpgradeBlock: NO\n" );
        

    ch_printf( ch, "Max. Modules Ext: %d Int:%d\n", ship->maxextmodules, ship->maxintmodules);

   ch_printf(ch, "Maxcargo: %d  Current Cargo: %d\n", ship->maxcargo, 0 );
    return;
}

void do_makeship( CHAR_DATA *ch, char *argument )
{
    SHIP_DATA *ship;
    std::string arg;
    std::string argstr = argument;
    
    argstr = one_argument( argstr, arg );
    
    if ( argstr.empty() )
    {
	send_to_char( "Usage: makeship <filename> <ship name>\n", ch );
	return;
    }

    CREATE( ship, SHIP_DATA, 1 );
    ship->game = ch->game;
    LINK( ship, first_ship, last_ship, next, prev );
    ship->game = ch->game;
    ship->name		= STRALLOC( argstr );
    ship->personalname		= STRALLOC( argstr );
    ship->description	= STRALLOC( "" );
    ship->owner 	= STRALLOC( "" );
    ship->copilot       = STRALLOC( "" );
    ship->pilot         = STRALLOC( "" );
    ship->home          = STRALLOC( "" );
    ship->type          = SHIP_CIVILIAN;
    ship->spaceobject = NULL;
    ship->energy = ship->maxenergy;
    ship->hull = ship->maxhull;
    ship->in_room=NULL;
    ship->next_in_room=NULL;
    ship->prev_in_room=NULL;
    ship->currjump=NULL;
    ship->target0=NULL;
    ship->maxextmodules=10;
    ship->maxintmodules=3;
    ship->modules=0;

    ship->filename = str_dup( arg );
    save_ship( ship );
    write_ship_list( ship->game );
	
}

void do_copyship( CHAR_DATA *ch, char *argument )
{
    SHIP_DATA *ship;
    SHIP_DATA *old;
    std::string arg;
    std::string arg2;
    std::string argstr = argument;

    argstr = one_argument( argstr, arg );
    argstr = one_argument( argstr, arg2 );
    
    if ( argstr.empty() )
    {
	send_to_char( "Usage: copyship <oldshipname> <filename> <newshipname>\n", ch );
	return;
    }

    old = get_ship ( ch->game, arg );
    
    if (!old)
    {
	send_to_char( "Thats not a ship!\n", ch );
	return;
    }

    CREATE( ship, SHIP_DATA, 1 );
    ship->game = ch->game;
    LINK( ship, first_ship, last_ship, next, prev );

    ship->name		= STRALLOC( argstr );
    ship->description	= STRALLOC( "" );
    ship->owner 	= STRALLOC( "" );
    ship->copilot       = STRALLOC( "" );
    ship->pilot         = STRALLOC( "" );
    ship->home          = STRALLOC( "" );
    ship->type          = old->type;
    ship->shipclass         = old->shipclass;
    ship->lasers        = old->lasers  ;
    ship->maxshield        = old->maxshield  ;
    ship->maxhull        = old->maxhull  ;
    ship->maxenergy        = old->maxenergy  ;
    ship->hyperspeed        = old->hyperspeed  ;
    ship->realspeed        = old->realspeed  ;
    ship->manuever        = old->manuever  ;
    ship->in_room=NULL;
    ship->next_in_room=NULL;
    ship->prev_in_room=NULL;
    ship->currjump=NULL;
    ship->target0=NULL;

    ship->filename         = str_dup(arg2);
    save_ship( ship );
    write_ship_list( ship->game );
}

void do_ships( CHAR_DATA *ch, char *argument )
{
    SHIP_DATA *ship;
    std::string buf;
    std::string pilottype;
    std::string pilottype2;
    int count;
    bool owned, set;
    
    if ( !IS_NPC(ch) )
    {
      count = 0;
      send_to_pager( "&YThe following ships you have pilot access to:\n", ch );
      send_to_pager( "\n&WShip                                                   Owner\n",ch);
      for ( ship = first_ship; ship; ship = ship->next )
      {
      	owned = FALSE, set = FALSE;
        if ( str_cmp(ship->owner, ch->name) )
        {
           if ( !check_pilot( ch, ship ) || !str_cmp(ship->owner, "public") || !str_cmp(ship->owner, "trainer") )
               continue;
        }
        
        if( ship->shipclass > SHIP_PLATFORM )
          continue;

        if (ship->type == MOB_SHIP)
           continue;
        else if (ship->type == SHIP_REBEL)
           set_pager_color( AT_BLOOD, ch );
        else if (ship->type == SHIP_IMPERIAL)
           set_pager_color( AT_DGREEN, ch );
        else
          set_pager_color( AT_BLUE, ch );

       if( !str_cmp(ship->owner, ch->name ) )
       {
         pilottype2 = "Owner";
         owned = TRUE;
         set = TRUE;
       }
       if( !set && !str_cmp( ship->pilot, ch->name ) )
       {
         pilottype2 = "Pilot";
         set = TRUE;
       }
       if( !set && !str_cmp( ship->copilot, ch->name ) )
       {
         pilottype2 = "Co-Pilot";
         set = TRUE;
       }
       if( !set )
       {
         pilottype2 = "Clan-Pilot";
         set = TRUE;
       }

       if( !owned )
         pilottype = "(" + pilottype2 + ") - " + ship->owner;
       else
         pilottype = "(" + pilottype2 + ")";
	
       buf = std::string(ship->name) + " (" + std::string(ship->personalname) + ")";


        if  ( ship->in_room )
          pager_printf( ch, "%-35s (%s) \n&R&W- %-24s&R&w \n", buf.c_str(), ship->in_room->name, pilottype.c_str() );
        else
          pager_printf( ch, "%-35s (%.0f %.0f %.0f) \n&R&W- %-35s&R&w\n", buf.c_str(), ship->vx, ship->vy, ship->vz, pilottype.c_str() );

        count++;
      }

      if ( !count )
      {
        send_to_pager( "There are no ships owned by you.\n", ch );
      }

    }


    count =0;
    send_to_pager( "&Y\nThe following ships are docked here:\n", ch );

    send_to_pager( "\n&WShip                               Owner          Cost/Rent\n", ch );
    for ( ship = first_ship; ship; ship = ship->next )
    {   
        if ( ship->location != ch->in_room->vnum || ship->shipclass > SHIP_PLATFORM)
               continue;

        if (ship->type == MOB_SHIP)
           continue;
        else if (ship->type == SHIP_REBEL)
           set_pager_color( AT_BLOOD, ch );
        else if (ship->type == SHIP_IMPERIAL)
           set_pager_color( AT_DGREEN, ch );
        else
          set_pager_color( AT_BLUE, ch );

       buf = std::string(ship->name) + " (" + std::string(ship->personalname) + ")";
        
        pager_printf( ch, "%-35s %-15s", buf.c_str(), ship->owner );
        if (ship->type == MOB_SHIP || ship->shipclass == SHIP_PLATFORM )
        {
          pager_printf( ch, "\n");
          continue;
        }
        if ( !str_cmp(ship->owner, "Public") )
        { 
          pager_printf( ch, "%ld to rent.\n", get_ship_value(ship)/100 ); 
        }
        else if ( str_cmp(ship->owner, "") )
          pager_printf( ch, "%s", "\n" );
        else
           pager_printf( ch, "%ld to buy.\n", get_ship_value(ship) ); 
        
        count++;
    }

    if ( !count )
    {
        send_to_pager( "There are no ships docked here.\n", ch );
    }
}

void do_speeders( CHAR_DATA *ch, char *argument )
{
    SHIP_DATA *ship;
    int count;
    std::string buf;
    std::string pilottype;
    std::string pilottype2;
    bool owned, set;

    if ( !IS_NPC(ch) )
    {
      count = 0;
      send_to_pager( "&YThe following ships you have pilot access to:\n", ch );
      send_to_pager( "\n&WShip                                                   Owner\n",ch);
      for ( ship = first_ship; ship; ship = ship->next )
      {
      	owned = FALSE, set = FALSE;
        if ( str_cmp(ship->owner, ch->name) )
        {
           if ( !check_pilot( ch, ship ) || !str_cmp(ship->owner, "public") || !str_cmp(ship->owner, "trainer") )
               continue;
        }
        
        if( ship->shipclass <= SHIP_PLATFORM )
          continue;

        if (ship->type == MOB_SHIP)
           continue;
        else if (ship->type == SHIP_REBEL)
           set_pager_color( AT_BLOOD, ch );
        else if (ship->type == SHIP_IMPERIAL)
           set_pager_color( AT_DGREEN, ch );
        else
          set_pager_color( AT_BLUE, ch );

       if( !str_cmp(ship->owner, ch->name ) )
       {
         pilottype2 = "Owner";
         owned = TRUE;
         set = TRUE;
       }
       if( !set && !str_cmp( ship->pilot, ch->name ) )
       {
         pilottype2 = "Pilot";
         set = TRUE;
       }
       if( !set && !str_cmp( ship->copilot, ch->name ) )
       {
         pilottype2 = "Co-Pilot";
         set = TRUE;
       }
       if( !set )
       {
         pilottype2 = "Clan-Pilot";
         set = TRUE;
       }

       if( !owned )
         pilottype = "(" + pilottype2 + ") - " + ship->owner;
       else
         pilottype = "(" + pilottype2 + ")";
	
       buf = std::string(ship->name) + " (" + std::string(ship->personalname) + ")";

        if  ( ship->in_room )
          pager_printf( ch, "%-35s (%s) \n&R&W- %-24s&R&w \n", buf.c_str(), ship->in_room->name, pilottype.c_str() );
        else
          pager_printf( ch, "%-35s (%.0f %.0f %.0f) \n&R&W- %-35s&R&w\n", buf.c_str(), ship->vx, ship->vy, ship->vz, pilottype.c_str() );

        count++;
      }

      if ( !count )
      {
        send_to_pager( "There are no ships owned by you.\n", ch );
      }

    }


    
    count =0;
    send_to_pager( "&Y\nThe following vehicles are parked here:\n", ch );
    
    send_to_pager( "\n&WVehicle                            Owner          Cost/Rent\n", ch );
    for ( ship = first_ship; ship; ship = ship->next )
    {   
        if ( ship->location != ch->in_room->vnum || ship->shipclass <= SHIP_PLATFORM)
               continue;
               
        if (ship->type == MOB_SHIP)
           continue;
        else if (ship->type == SHIP_REBEL)
           set_pager_color( AT_BLOOD, ch );
        else if (ship->type == SHIP_IMPERIAL)
           set_pager_color( AT_DGREEN, ch );
        else
          set_pager_color( AT_BLUE, ch );
        

        buf = std::string(ship->name) + " (" + std::string(ship->personalname) + ")";
        pager_printf( ch, "%-35s%-15s ", buf.c_str(), ship->owner );
        
        if ( !str_cmp(ship->owner, "Public") )
        { 
          pager_printf( ch, "%ld to rent.\n", get_ship_value(ship)/100 ); 
        }
        else if ( str_cmp(ship->owner, "") )
          pager_printf( ch, "%s", "\n" );
        else
           pager_printf( ch, "%ld to buy.\n", get_ship_value(ship) ); 
        
        count++;
    }

    if ( !count )
    {
        send_to_pager( "There are no sea air or land vehicles here.\n", ch );
    }
}

void do_allspeeders( CHAR_DATA *ch, char *argument )
{
    SHIP_DATA *ship;
    int count = 0;
    std::string buf;

      count = 0;
      send_to_pager( "&Y\nThe following sea/land/air vehicles are currently formed:\n", ch );
    
      send_to_pager( "\n&WVehicle                            Owner\n", ch );
      for ( ship = first_ship; ship; ship = ship->next )
      {   
        if ( ship->shipclass <= SHIP_PLATFORM ) 
           continue; 
      
        if (ship->type == MOB_SHIP)
           continue;
        else if (ship->type == SHIP_REBEL)
           set_pager_color( AT_BLOOD, ch );
        else if (ship->type == SHIP_IMPERIAL)
           set_pager_color( AT_DGREEN, ch );
        else
          set_pager_color( AT_BLUE, ch );
        
        buf = std::string(ship->name) + " (" + std::string(ship->personalname) + ")";
        pager_printf( ch, "%-35s%-15s ", buf.c_str(), ship->owner );

        if ( !str_cmp(ship->owner, "Public") )
        { 
          pager_printf( ch, "%ld to rent.\n", get_ship_value(ship)/100 ); 
        }
        else if ( str_cmp(ship->owner, "") )
          pager_printf( ch, "%s", "\n" );
        else
           pager_printf( ch, "%ld to buy.\n", get_ship_value(ship) );
        
        count++;
      }
    
      if ( !count )
      {
        send_to_pager( "There are none currently formed.\n", ch );
	return;
      }
    
}

void do_allships( CHAR_DATA *ch, char *argument )
{
    SHIP_DATA *ship;
    std::string buf;
    int count = 0;
    bool unowned = FALSE, mobship = FALSE, checkowner = FALSE, data = FALSE;
    int type = -1;
 
    if ( !str_cmp( argument, "unowned" ) )
      unowned = TRUE;
    else if ( !str_cmp( argument, "imperial" ) )
      type = SHIP_IMPERIAL;
    else if ( !str_cmp( argument, "rebel" ) )
      type = SHIP_REBEL;
    else if ( !str_cmp( argument, "civilian" ) )
      type = SHIP_CIVILIAN;
    else if ( !str_cmp( argument, "mob" ) )
      mobship = TRUE;
    else if (!str_cmp (argument, "data"))
      data = TRUE;
    else if (!argument || argument[0] == '\0' || !str_cmp (argument, ""))
      ;
    else
      checkowner = TRUE;

    if ( data )
    {
      send_to_pager ( "\nPersonal name            Filename       Owner     \n", ch);
      for ( ship = first_ship; ship; ship = ship->next )
        pager_printf( ch, "%25s | %15s | %10s\n", ship->personalname, ship->filename, ship->owner );
      return;
    }
      
      count = 0;
      send_to_pager( "&Y\nThe following ships are currently formed:\n", ch );
    
      send_to_pager( "\n&WShip                               Owner\n", ch );
      
      if ( IS_IMMORTAL( ch ) && !unowned && !checkowner && type < 0)
        for ( ship = first_ship; ship; ship = ship->next )
           if (ship->type == MOB_SHIP && ship->shipclass != SHIP_DEBRIS )
           {
              buf = std::string(ship->name) + " (" + std::string(ship->personalname) + ")";
              pager_printf( ch, "&w%-35s %-10s\n", buf.c_str(), ship->owner );
           }
      if( !mobship )       
        for ( ship = first_ship; ship; ship = ship->next )
        {   
          if ( ship->shipclass > SHIP_PLATFORM ) 
            continue; 
        
          if( unowned && str_cmp(ship->owner, "") )
            continue;
          if( checkowner && str_cmp(ship->owner, argument) )
            continue;
            
          if( type >= 0 && ship->type != type )
            continue;  
        
          if (ship->type == MOB_SHIP)
            continue;
          else if (ship->type == SHIP_REBEL)
            set_pager_color( AT_BLOOD, ch );
          else if (ship->type == SHIP_IMPERIAL)
            set_pager_color( AT_DGREEN, ch );
          else
            set_pager_color( AT_BLUE, ch );
          buf = std::string(ship->name) + " (" + std::string(ship->personalname) + ")";
          pager_printf( ch, "&w%-35s %-15s\n", buf.c_str(), ship->owner );
          if (ship->type == MOB_SHIP || ship->shipclass == SHIP_PLATFORM )
          {
            pager_printf( ch, "\n");
            continue;
          }
          if ( !str_cmp(ship->owner, "Public") )
          { 
            pager_printf( ch, "%ld to rent.\n", get_ship_value(ship)/100 ); 
          }
          else if ( str_cmp(ship->owner, "") )
            pager_printf( ch, "%s", "\n" );
          else
            pager_printf( ch, "&W%-10s%ld to buy.&R&w\n", "", get_ship_value(ship) ); 
          
          count++;
        }
    
      if ( !count )
      {
        send_to_pager( "There are no ships currently formed.\n", ch );
	      return;
      }
    
}


void ship_to_spaceobject( SHIP_DATA *ship , SPACE_DATA *spaceobject )
{
     if ( spaceobject == NULL )
        return;
     
     if ( ship == NULL )
        return;
     
     ship->spaceobject = spaceobject;
     ship->dirty = true;
        
}

void new_missile( SHIP_DATA *ship , SHIP_DATA *target , CHAR_DATA *ch , int missiletype )
{
     SPACE_DATA *spaceobject;
     MISSILE_DATA *missile;

     if ( ship  == NULL )
        return;

     if ( target  == NULL )
        return;

     if ( ( spaceobject = ship->spaceobject ) == NULL )
        return;
          
     CREATE( missile, MISSILE_DATA, 1 );
     missile->game = ship->game;
     LINK( missile, first_missile, last_missile, next, prev );

     missile->target = target; 
     missile->fired_from = ship;
     if ( ch )
        missile->fired_by = STRALLOC( ch->name );
     else 
        missile->fired_by = STRALLOC( "" );
     missile->missiletype = missiletype;
     missile->age =0;
     if ( missile->missiletype == HEAVY_BOMB )
       missile->speed = 20;
     else if ( missile->missiletype == PROTON_TORPEDO ) 
       missile->speed = 200;
     else if ( missile->missiletype == CONCUSSION_MISSILE ) 
       missile->speed = 300;
     else 
       missile->speed = 50;
     
     missile->mx = ship->vx;
     missile->my = ship->vy;
     missile->mz = ship->vz;
            
     missile->spaceobject = spaceobject;
        
}

void ship_from_spaceobject( SHIP_DATA *ship , SPACE_DATA *spaceobject )
{

     if ( spaceobject == NULL )
        return;

     if ( ship == NULL )
        return;
     
     ship->spaceobject = NULL;
     ship->dirty = true;

}

void extract_missile( MISSILE_DATA *missile )
{
    SPACE_DATA *spaceobject;

     if ( missile == NULL )
        return;

     if ( ( spaceobject = missile->spaceobject ) != NULL )
      missile->spaceobject = NULL;
     
     UNLINK( missile, first_missile, last_missile, next, prev );
     
     missile->target = NULL; 
     missile->fired_from = NULL;
     if (  missile->fired_by )
        STRFREE( missile->fired_by );
     
     DISPOSE( missile );
          
}

bool is_rental( CHAR_DATA *ch , SHIP_DATA *ship )
{
   if ( !str_cmp("Public",ship->owner) )
          return TRUE;
   if ( !str_cmp("Trainer",ship->owner) )
          return TRUE;
   if ( ship->shipclass == SHIP_TRAINER )
          return TRUE;

   return FALSE;
}

bool check_pilot( CHAR_DATA *ch , SHIP_DATA *ship )
{
   if ( !str_cmp(ch->name,ship->owner) || !str_cmp(ch->name,ship->pilot)
   || !str_cmp(ch->name,ship->copilot) || !str_cmp("Public",ship->owner)
   || !str_cmp("Trainer", ship->owner) )
      return TRUE;

   if ( !IS_NPC(ch) && ch->pcdata && ch->pcdata->clan )
   {
      if ( !str_cmp(ch->pcdata->clan->name,ship->owner) )
      {
        if ( !str_cmp(ch->pcdata->clan->leader,ch->name) )
          return TRUE;
        if ( !str_cmp(ch->pcdata->clan->number1,ch->name) )
          return TRUE;
        if ( !str_cmp(ch->pcdata->clan->number2,ch->name) )
          return TRUE;
        if ( ch->pcdata->bestowments && is_name( "pilot", ch->pcdata->bestowments) )
          return TRUE;
      }
      if ( !str_cmp(ch->pcdata->clan->name,ship->pilot) )
      {
        if ( !str_cmp(ch->pcdata->clan->leader,ch->name) )
          return TRUE;
        if ( !str_cmp(ch->pcdata->clan->number1,ch->name) )
          return TRUE;
        if ( !str_cmp(ch->pcdata->clan->number2,ch->name) )
          return TRUE;
        if ( ch->pcdata->bestowments && is_name( "pilot", ch->pcdata->bestowments) )
          return TRUE;
      }
      if ( !str_cmp(ch->pcdata->clan->name,ship->copilot) )
      {
        if ( !str_cmp(ch->pcdata->clan->leader,ch->name) )
          return TRUE;
        if ( !str_cmp(ch->pcdata->clan->number1,ch->name) )
          return TRUE;
        if ( !str_cmp(ch->pcdata->clan->number2,ch->name) )
          return TRUE;
        if ( ch->pcdata->bestowments && is_name( "pilot", ch->pcdata->bestowments) )
          return TRUE;
      }


   }

   return FALSE;
}

bool extract_ship( SHIP_DATA *ship )
{   
    ROOM_INDEX_DATA *room;
    
    if ( ( room = ship->in_room ) != NULL )
    {
        UNLINK( ship, room->first_ship, room->last_ship, next_in_room, prev_in_room );
        ship->in_room = NULL;
    }
    return TRUE;
}

bool damage_ship_ch( SHIP_DATA *ship , int min , int max , CHAR_DATA *ch )
{
    sh_int ionFactor = 1;
    int damage , shield_dmg;
    long xp;
    bool ions = FALSE;
//  char  logbuf[MAX_STRING_LENGTH];
    std::string  buf;
    TURRET_DATA *turret;
    int turretnum;

    if ( min < 0 && max < 0 )
    {
      ions = TRUE;
      damage = number_range( max*(-1), min*(-1) );
    }
    else
      damage = number_range( min , max );

    if ( ions == TRUE )
      ionFactor = 2;

    xp = ( exp_level( ch->skill_level[PILOTING_ABILITY]+1) - exp_level( ch->skill_level[PILOTING_ABILITY]) ) / 25 ;
    xp = UMIN( get_ship_value( ship ) /100 , xp ) ;
    gain_exp( ch , xp , PILOTING_ABILITY );

    if ( ship->shield > 0 )
    {
      shield_dmg = UMIN( ship->shield , damage );
 	   damage -= shield_dmg;
  	   ship->shield -= shield_dmg;
    	if ( ship->shield == 0 )
    	  echo_to_cockpit( AT_BLOOD , ship , "Shields down..." );
    }

    if ( damage > 0 )
    {
      if ( number_range(1, 100) <= 5*ionFactor && ship->shipstate != SHIP_DISABLED )
      {
         echo_to_cockpit( AT_BLOOD + AT_BLINK , ship , "Ships Drive DAMAGED!" );
         ship->shipstate = SHIP_DISABLED;
      }

      if ( number_range(1, 100) <= 5*ionFactor && ship->missilestate != MISSILE_DAMAGED && ship->mod->launchers > 0 )
      {
         echo_to_room( AT_BLOOD + AT_BLINK , get_room_index(ship->gunseat) , "Ships Missile Launcher DAMAGED!" );
         ship->missilestate = MISSILE_DAMAGED;
      }
      if ( number_range(1, 100) <= 2*ionFactor && ship->statet0 != LASER_DAMAGED )
      {
         echo_to_room( AT_BLOOD + AT_BLINK , get_room_index(ship->gunseat) , "Lasers DAMAGED!" );
         ship->statet0 = LASER_DAMAGED;
      }
      if ( ship->first_turret )
        for( turret = ship->first_turret, turretnum = 0; turret; turret = turret->next, turretnum++ )
          if ( number_range(1, 100) <= 5*ionFactor && turret->state != LASER_DAMAGED )
      {
         buf = str_printf("Turret%d DAMAGED!\n", turretnum);
         echo_to_room( AT_BLOOD + AT_BLINK , get_room_index(turret->roomvnum) , buf );
         echo_to_room( AT_BLOOD + AT_BLINK , get_room_index(ship->cockpit) , buf );
         echo_to_room( AT_BLOOD + AT_BLINK , get_room_index(ship->engineroom) , buf );
         turret->state = LASER_DAMAGED;
      }
      if ( number_range(1, 100) <= 5*ionFactor && ship->statettractor != LASER_DAMAGED && ship->mod->tractorbeam )
      {
         echo_to_room( AT_BLOOD + AT_BLINK , get_room_index(ship->pilotseat) , "Tractorbeam DAMAGED!" );
         ship->statettractor = LASER_DAMAGED;
      }
      if ( ions == FALSE )
      {
        ship->hull -= damage*5;
      }
    }

    if ( ship->hull <= 0 )
    {
        buf = str_printf("%s(%s)", ship->name, ship->personalname);
        log_printf( "%s was just destroyed by %s." , buf.c_str(), ch->name );


       xp =  ( exp_level( ch->skill_level[PILOTING_ABILITY]+1) - exp_level( ch->skill_level[PILOTING_ABILITY]) );
       xp = UMIN( get_ship_value( ship ) , xp );
       gain_exp( ch , xp , PILOTING_ABILITY);
       ch_printf( ch, "&WYou gain %ld piloting experience!\n", xp );

       if ( destroy_ship( ship , ch ) )
         return TRUE;

       return FALSE;
    }

    if ( ship->hull <= ship->mod->maxhull/20 )
       echo_to_cockpit( AT_BLOOD+ AT_BLINK , ship , "WARNING! Ship hull severely damaged!" );

    return FALSE;

}

bool damage_ship( SHIP_DATA *ship , SHIP_DATA *assaulter, int min , int max )
{
    int damage , shield_dmg;
    std::string buf;
//  char logbuf[MAX_STRING_LENGTH];
    sh_int ionFactor = 1;
    bool ions = FALSE;
    TURRET_DATA *turret;
    int turretnum;

    if ( min < 0 && max < 0 )
    {
      ions = TRUE;
      damage = number_range( max*(-1), min*(-1) );
    }
    else
      damage = number_range( min , max );

    if ( ions == TRUE )
      ionFactor = 2;

    if ( ship->shield > 0 )
    {
      shield_dmg = UMIN( ship->shield , damage );
 	   damage -= shield_dmg;
  	   ship->shield -= shield_dmg;
    	if ( ship->shield == 0 )
    	  echo_to_cockpit( AT_BLOOD , ship , "Shields down..." );
    }

    if ( ship->shield > 0 )
    {
        shield_dmg = UMIN( ship->shield , damage );
    	damage -= shield_dmg;
    	ship->shield -= shield_dmg;
    	if ( ship->shield == 0 )
    	  echo_to_cockpit( AT_BLOOD , ship , "Shields down..." );    	  
    }
    
    if ( damage > 0 )
    {
      if ( number_range(1, 100) <= 5*ionFactor && ship->shipstate != SHIP_DISABLED )
      {
         echo_to_cockpit( AT_BLOOD + AT_BLINK , ship , "Ships Drive DAMAGED!" );
         ship->shipstate = SHIP_DISABLED;
      }

      if ( number_range(1, 100) <= 5*ionFactor && ship->missilestate != MISSILE_DAMAGED && ship->mod->launchers> 0 )
      {
         echo_to_room( AT_BLOOD + AT_BLINK , get_room_index(ship->gunseat) , "Ships Missile Launcher DAMAGED!" );
         ship->missilestate = MISSILE_DAMAGED;
      }
      if ( number_range(1, 100) <= 2*ionFactor && ship->statet0 != LASER_DAMAGED )
      {
         echo_to_room( AT_BLOOD + AT_BLINK , get_room_index(ship->gunseat) , "Lasers DAMAGED!" );
         ship->statet0 = LASER_DAMAGED;
      }
      if ( ship->first_turret )
        for( turret = ship->first_turret, turretnum = 0; turret; turret = turret->next, turretnum++ )
          if ( number_range(1, 100) <= 5*ionFactor && turret->state != LASER_DAMAGED )
      {
	       buf = str_printf("Turret%d DAMAGED!\n", turretnum);
         echo_to_room( AT_BLOOD + AT_BLINK , get_room_index(turret->roomvnum) , buf );
         echo_to_room( AT_BLOOD + AT_BLINK , get_room_index(ship->cockpit) , buf );
         echo_to_room( AT_BLOOD + AT_BLINK , get_room_index(ship->engineroom) , buf );
         turret->state = LASER_DAMAGED;
      }
      if ( number_range(1, 100) <= 5*ionFactor && ship->statettractor != LASER_DAMAGED && ship->mod->tractorbeam )
      {
         echo_to_room( AT_BLOOD + AT_BLINK , get_room_index(ship->pilotseat) , "Tractorbeam DAMAGED!" );
         ship->statettractor = LASER_DAMAGED;
      }
      if ( ions == FALSE )
      {
        ship->hull -= damage*5;
      }
    }

    ship->hull -= damage*5;
    
    if ( ship->hull <= 0 )
    {
      buf = str_printf("%s(%s)", ship->name, ship->personalname);
      log_printf( "%s was just destroyed by %s." , buf.c_str(), (assaulter ? assaulter->personalname : "a collision" ));
      if ( destroy_ship( ship , NULL ) )
        return TRUE;
      return FALSE;
    }

    if ( ship->hull <= ship->mod->maxhull/20 )
       echo_to_cockpit( AT_BLOOD+ AT_BLINK , ship , "WARNING! Ship hull severely damaged!" );
       
    return FALSE;
}

bool destroy_ship( SHIP_DATA *ship , CHAR_DATA *ch )
{
    std::string buf;
    std::string buf2;
//  char logbuf[MAX_STRING_LENGTH];
    int  roomnum;
    ROOM_INDEX_DATA *room;
    OBJ_DATA *robj;
    CHAR_DATA *rch;
    SHIP_DATA *lship;
    TURRET_DATA *turret;

    if (!ship)
      return TRUE;

    buf = str_printf("%s explodes in a blinding flash of light!", ship->name);
    echo_to_system( AT_WHITE + AT_BLINK , ship , buf , NULL );

    if ( ship->shipclass == FIGHTER_SHIP )

    echo_to_ship( AT_WHITE + AT_BLINK , ship , "A blinding flash of light burns your eyes...");
    echo_to_ship( AT_WHITE , ship , "But before you have a chance to scream...\nYou are ripped apart as your spacecraft explodes...");

#ifdef NODEATH
    resetship(ship);
    makedebris(ship);
    return TRUE;
#endif
#ifdef NODEATHSHIP
    resetship(ship);
    makedebris(ship);
    return TRUE;
#endif

    if ( !str_cmp("Trainer", ship->owner))
    {
      resetship(ship);
      return TRUE;
    }

/*    explode_ship( ship ); */

    makedebris(ship);

    for ( roomnum = ship->get_firstroom() ; roomnum <= ship->get_lastroom() ; roomnum++ )
    {
        room = get_room_index(roomnum);

        if (room != NULL)
        {
         rch = room->first_person;
         while ( rch )
         {
            if ( IS_IMMORTAL(rch) )
            {
                 char_from_room(rch);
                 char_to_room( rch, get_room_index(wherehome(rch)) );
            }
            else
            {
              if ( ch )
                  raw_kill( ch , rch );
               else
                  raw_kill( rch , rch );
            }
            rch = room->first_person;
         }

         for ( robj = room->first_content ; robj ; robj = robj->next_content )
         {
           separate_obj( robj );
           extract_obj( robj );
         }
        }

    }

    for ( lship = first_ship; lship; lship = lship->next )
    {   
       if ( lship->docked && lship->docked == ship )
       {
        if ( lship->location )
          lship->shipstate = SHIP_LANDED;
        else                
          lship->shipstate = SHIP_READY;
        lship->docked = NULL;
        ship->docking = SHIP_READY;
       }    

       if ( !(ship->hanger) || (lship->location != ship->hanger) )
               continue;
       if ( ch )
       {
        buf2 = str_printf("%s(%s)", lship->name, lship->personalname);
        log_printf( "%s was just destroyed by %s." , buf2.c_str(), ch->name );
       }
       else
       {
        buf2 = str_printf("%s(%s)", lship->name, lship->personalname);
        log_printf( "%s was just destroyed by a mob ship." , buf2.c_str());
       }

        destroy_ship( lship, ch );
    }  

  for( lship = first_ship; lship; lship = lship->next )
  {
    if( lship->target0 == ship )
      lship->target0 = NULL;
    for( turret = lship->first_turret; turret; turret = turret->next )
      if( turret->target == ship )
        turret->target = NULL;
    if( lship->tractoredby == ship )
      lship->tractoredby = NULL;
    if( lship->tractored == ship )
      lship->tractored = NULL;
  }

  if ( ( ship->shipclass < CAPITAL_SHIP && ship->type != MOB_SHIP ) || ship->shipclass == SHIP_DEBRIS || is_rental(ch, ship) )
    shipdelete( ship, TRUE );
  else if ( ship->templatestring && ship->templatestring[0] != '\0' )
    shipdelete( ship, TRUE );
  else
    resetship ( ship );
  return TRUE;
  
}

bool ship_to_room(SHIP_DATA *ship , int vnum )
{
    ROOM_INDEX_DATA *shipto;
    
    if ( (shipto=get_room_index(vnum)) == NULL )
            return FALSE;
    LINK( ship, shipto->first_ship, shipto->last_ship, next_in_room, prev_in_room );
    ship->in_room = shipto; 
    return TRUE;
}


void do_board( CHAR_DATA *ch, char *argument )
{
//  ROOM_INDEX_DATA *fromroom;
    ROOM_INDEX_DATA *toroom;
    SHIP_DATA *ship;
    SHIP_DATA *eShip = NULL;

    if ( ( ship = ship_from_entrance( ch->game, ch->in_room->vnum ) ) != NULL )
    {
      eShip = get_ship( ch->game, argument );
      if( !eShip || !eShip->docked || eShip->docked != ship )
        eShip = NULL;
        
      if ( !eShip && ship->docked == NULL )
        for( eShip = first_ship; eShip; eShip = eShip->next )
          if( eShip->docked && eShip->docked == ship && ( !argument || argument[0] == '\0'))
            break;

      if( !eShip && ship->docked && ( !argument || argument[0] == '\0') )
        eShip = ship->docked;

      if ( eShip && ship->docking == SHIP_READY && eShip->docking == SHIP_READY )
      {
        send_to_char( "You are not docked to a ship.\n", ch );
        return;
      }
      if ( eShip && ship->docking != SHIP_DOCKED && eShip->docking != SHIP_DOCKED )
      {
        send_to_char( "The docking manuever is still being completed.\n", ch );
        return;
      }

      if( eShip )
      {
          act( AT_PLAIN, "$n boards $T.", ch,
            NULL, eShip->name , TO_ROOM );
          act( AT_PLAIN, "You board $T.", ch,
            NULL, eShip->name , TO_CHAR );

          toroom = get_room_index( eShip->entrance );

          if ( !toroom )
            toroom = eShip->cockpitroom;

          char_from_room( ch );
          char_to_room( ch, toroom );

          act( AT_PLAIN, "$n boards the ship.", ch,
            NULL, argument , TO_ROOM );

          do_look( ch , "auto" );
          return;
      }
    }


    if ( !argument || argument[0] == '\0')
    {
        send_to_char( "Board what?\n", ch );
        return;
    }


    if ( ( ship = ship_in_room( ch->in_room , argument ) ) == NULL )
    {
              act( AT_PLAIN, "I see no $T here.", ch, NULL, argument, TO_CHAR );
            return;
    }

    if ( BV_IS_SET( ch->act, ACT_MOUNTED ) && ( ch->position == POS_MOUNTED ) )
    {
            act( AT_PLAIN, "You can't go in there riding THAT.", ch, NULL, argument, TO_CHAR );
            return;
    } 

//   fromroom = ch->in_room;
    toroom = get_room_index( ship->entrance );
    if( !toroom ) 
      toroom = ship->cockpitroom;

    if ( ( toroom ) != NULL )
   	{
   	   if ( ! ship->hatchopen )
   	   {
   	      send_to_char( "&RThe hatch is closed!\n", ch);
   	      return;
   	   }

           if ( toroom->tunnel > 0 )
           {
	        CHAR_DATA *ctmp;
	        int count = 0;

	       for ( ctmp = toroom->first_person; ctmp; ctmp = ctmp->next_in_room )
	       if ( ++count >= toroom->tunnel )
	       {
                  send_to_char( "There is no room for you in there.\n", ch );
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
            char_to_room( ch , toroom );
            act( AT_PLAIN, "$n enters the ship.", ch,
              NULL, argument , TO_ROOM );
            do_look( ch , "auto" );

        }
        else
          send_to_char("That ship has no entrance!\n", ch);
}

bool rent_ship( CHAR_DATA *ch , SHIP_DATA *ship )
{   

    long price;

    if ( IS_NPC ( ch ) )
       return FALSE;

    price = get_ship_value( ship )/100;
    
    if( ship->cockpitroom )
     price = 0;
   
       if ( ch->gold < price )
       {
         ch_printf(ch, "&RRenting this ship costs %ld. You don't have enough credits!\n" , price );
         return FALSE;
       }

       ch->gold -= price;
       ch_printf(ch, "&GYou pay %ld credits to rent the ship.\n" , price );   
       return TRUE;
     
}

void do_leaveship( CHAR_DATA *ch, char *argument )
{
    ROOM_INDEX_DATA *fromroom;
    ROOM_INDEX_DATA *toroom;
    SHIP_DATA *ship;
    
    fromroom = ch->in_room;
    
    if  ( (ship = ship_from_entrance(ch->game,fromroom->vnum)) == NULL )
    {
        send_to_char( "I see no exit here.\n" , ch );
        return;
    }   
    
    if  ( ship->shipclass == SHIP_PLATFORM )
    {
        send_to_char( "You can't do that here.\n" , ch );
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
    	send_to_char("&RYou need to open the hatch first\n" , ch );
    	return;
    }
    
    if ( ( toroom = get_room_index( ship->location ) ) != NULL )
    {
        act( AT_PLAIN, "$n exits the ship.", ch,
          NULL, argument , TO_ROOM );
        act( AT_PLAIN, "You exit the ship.", ch,
          NULL, argument , TO_CHAR );
   	    char_from_room( ch );
   	    char_to_room( ch , toroom );
   	    act( AT_PLAIN, "$n steps out of a ship.", ch,
		      NULL, argument , TO_ROOM );
            do_look( ch , "auto" );
        if ( ship->cockpitroom && !ship->cockpitroom->first_person && !str_cmp( ship->owner, "Public" ) )
        {
            if ( BV_IS_SET( ch->in_room->room_flags, ROOM_CAN_LAND ) )
            {
                    echo_to_room( AT_YELLOW , toroom , "The newly arrived rental is rolled into storage.\n" );
                    dumpship( ship, toroom->vnum );
              storeship(ship);
            }
        }
     }       
     else
        send_to_char ( "The exit doesn't seem to be working properly.\n", ch );  
}

void do_launch( CHAR_DATA *ch, char *argument )
{
    int chance, shipclass; 
    long price = 0;

    scmd_data ctx;
    ctx.rest = argument;

    ctx.rest = one_argument( ctx.rest , ctx.arg1);
    ctx.rest = one_argument( ctx.rest , ctx.arg2);

    if( !ctx.arg1.empty() )
    {
      //destin = spaceobject_from_name(ch->game, ctx.arg1 );
      shipclass = strtoi(ctx.arg2);
    }

    if (!space_require_ship_from_cockpit( ch , ctx )) return;
    if (!space_require_ship_from_pilotseat( ch , ctx )) return;
    if (!space_require_ship_movement( ch , ctx )) return;
    if (!space_require_ship_tractoree_ed( ch , ctx )) return;    
    if (!space_require_ship_not_docked( ch , ctx )) return;
    if (!space_require_ship_spaceship( ch , ctx )) return;
    if (!space_require_ship_ownership( ch , ctx )) return;
    if (!space_require_tractor_weaker( ch , ctx )) return;
    
    if (!space_require_ship_autopilot_off( ch , ctx )) return;
    if (!space_require_ship_docked( ch , ctx )) return;

    if (!space_require_ship_launch_ready_state( ch , ctx )) return;

    chance = space_chance_by_shipclass( ch, ctx.ship );

    if ( number_percent( ) < chance )
    {  
        if( 0 )
        {
            if(shipclass > 0 && shipclass < 4)
          /*modtrainer( ship, shipclass )*/;
            else
            {
                ch_printf(ch, "&RShip classes: fighter 1, midship 2, capital 3.\n");
                return;
            }
            return;
            if ( !(ctx.ship->destin) )
            {
                ch_printf(ch, "&RInvalid Trainer system.\n");
                return;
            }
        }

        if ( is_rental(ch,ctx.ship) )
          if( !rent_ship(ch,ctx.ship) )
              return; 
        if ( !is_rental(ch,ctx.ship) )
        {    
            if ( ctx.ship->shipclass == FIGHTER_SHIP )
                  price=2000;
            if ( ctx.ship->shipclass == MIDSIZE_SHIP )
              price=5000;
            if ( ctx.ship->shipclass == CAPITAL_SHIP )
              price=50000;
            
            price += ( ctx.ship->mod->maxhull-ctx.ship->hull );

            if (ctx.ship->shipstate == SHIP_DISABLED )
                price += 10000;
            if ( ctx.ship->missilestate == MISSILE_DAMAGED )
                price += 5000;
            if ( ctx.ship->statet0 == LASER_DAMAGED )
                price += 2500;
        }                
                    
        if( BV_IS_SET( ch->act, PLR_DONTAUTOFUEL ) )
        {
            if( ctx.ship->shipstate == SHIP_DISABLED )
              {
                  ch_printf(ch, "Your ship is disabled. You must repair it.\n");
                  return;
              }
            
            price = 100;
        }
          
        if ( ch->pcdata && ch->pcdata->clan && !str_cmp(ch->pcdata->clan->name,ctx.ship->owner) )
        {
          if ( ch->pcdata->clan->funds < price )
          {
              ch_printf(ch, "&R%s doesn't have enough funds to prepare this ship for launch.\n", ch->pcdata->clan->name );
              return;
          }

          ch->pcdata->clan->funds -= price;
          ctx.room = get_room_index( ctx.ship->location );
          if( ctx.room != NULL && ctx.room->area )
              boost_economy( ctx.room->area, price );
          ch_printf(ch, "&GIt costs %s %ld credits to ready this ship for launch.\n", ch->pcdata->clan->name, price );
        }
        else if ( str_cmp( ctx.ship->owner , "Public" ) )
        {
            if ( ch->gold < price )
            {
                ch_printf(ch, "&RYou don't have enough funds to prepare this ship for launch.\n");
                return;
            }

            ch->gold -= price;
            ctx.room = get_room_index( ctx.ship->location );
            if( ctx.room != NULL && ctx.room->area )
            boost_economy( ctx.room->area, price );
            ch_printf(ch, "&GYou pay %ld credits to ready the ship for launch.\n", price );

        }

        if( !BV_IS_SET( ch->act, PLR_DONTAUTOFUEL ) )
        {
              if(  ship_from_hanger(ctx.ship->game, ctx.ship->in_room->vnum) == NULL || ctx.ship->shipclass == SHIP_PLATFORM )
                            ctx.ship->energy = ctx.ship->mod->maxenergy;
              ctx.ship->shield = 0;
              ctx.ship->autorecharge = FALSE;
              ctx.ship->autotrack = FALSE;
              ctx.ship->autospeed = FALSE;
              ctx.ship->hull = ctx.ship->mod->maxhull;

              ctx.ship->missilestate = MISSILE_READY;
              ctx.ship->statet0 = LASER_READY;
              space_set_shipstate(ctx.ship, SHIP_LANDED);
          }
        
          if (ctx.ship->hatchopen)
          {
              ctx.ship->hatchopen = FALSE;
              echo_to_room( AT_YELLOW , get_room_index(ctx.ship->location) , str_printf("The hatch on %s closes." , ctx.ship->name) );
              echo_to_room( AT_YELLOW , get_room_index(ctx.ship->entrance) , "The hatch slides shut." );
              sound_to_room( get_room_index(ctx.ship->entrance) , "!!SOUND(door)" );
              sound_to_room( get_room_index(ctx.ship->location) , "!!SOUND(door)" );
          }
          set_char_color( AT_GREEN, ch );
          send_to_char( "Launch sequence initiated.\n", ch);
          act( AT_PLAIN, "$n starts up the ship and begins the launch sequence.", ch,
              NULL, ctx.rest , TO_ROOM );
          echo_to_ship( AT_YELLOW , ctx.ship , "The ship hums as it lifts off the ground.");
          echo_to_room( AT_YELLOW , get_room_index(ctx.ship->location) , str_printf("%s begins to launch.", ctx.ship->name) );
          echo_to_docked( AT_YELLOW , ctx.ship, "The ship shudders as it lifts off the ground." );
          space_set_shipstate(ctx.ship, SHIP_LAUNCH);
          ctx.ship->goalspeed = ctx.ship->mod->realspeed;
          ctx.ship->accel = get_acceleration(ctx.ship);
          space_learn_from_success( ch, ctx.ship );
          sound_to_ship(ctx.ship , "!!SOUND(xwing)" );  
          return;   	   	
    }
    set_char_color( AT_RED, ch );
    send_to_char("You fail to work the controls properly!\n",ch);
    space_learn_from_failure( ch, ctx.ship );
    return;	

}

void launchship( SHIP_DATA *ship )
{   
    std::string buf;
    SHIP_DATA *target;
    int plusminus;
    
    ship->spaceobject = NULL;
    ship_to_spaceobject( ship, spaceobject_from_vnum( ship->game, ship->location ) );
    
    
    if ( ship->spaceobject == NULL )
    {
       echo_to_room( AT_YELLOW , get_room_index(ship->pilotseat) , "Launch path blocked .. Launch aborted.");
       echo_to_ship( AT_YELLOW , ship , "The ship slowly sets back back down on the landing pad.");
       echo_to_room( AT_YELLOW , get_room_index(ship->location) , str_printf("%s slowly sets back down." ,ship->name) ); 
       ship->shipstate = SHIP_LANDED;
       return;
    }    
    
    if (ship->shipclass == MIDSIZE_SHIP)
    {
       sound_to_room( get_room_index(ship->location) , "!!SOUND(falcon)" );
       sound_to_ship(ship , "!!SOUND(falcon)" );
    }
    else if (ship->type == SHIP_IMPERIAL )
    {
       sound_to_ship(ship , "!!SOUND(tie)" );  
       sound_to_room( get_room_index(ship->location) , "!!SOUND(tie)" );
    }
    else
    {
       sound_to_ship(ship , "!!SOUND(xwing)" );  
       sound_to_room( get_room_index(ship->location) , "!!SOUND(xwing)" );
    }
    
    extract_ship(ship);    
    
    ship->location = 0;
    
    if (ship->shipstate != SHIP_DISABLED)
       ship->shipstate = SHIP_READY;
    
    plusminus = number_range ( -1 , 2 );
    if (plusminus > 0 )
        ship->hx = 1;
    else
        ship->hx = -1;
    
    plusminus = number_range ( -1 , 2 );
    if (plusminus > 0 )
        ship->hy = 1;
    else
        ship->hy = -1;
        
    plusminus = number_range ( -1 , 2 );
    if (plusminus > 0 )
        ship->hz = 1;
    else
        ship->hz = -1;
    
    if (ship->spaceobject && 
      ( ship->lastdoc == ship->spaceobject->doca  ||
        ship->lastdoc == ship->spaceobject->docb ||
        ship->lastdoc == ship->spaceobject->docc ) )
    {
       ship->vx = ship->spaceobject->xpos;
       ship->vy = ship->spaceobject->ypos;
       ship->vz = ship->spaceobject->zpos;
    }
    else
    {
       for ( target = first_ship; target; target = target->next )
       {
          if (ship->lastdoc == target->hanger)
          {
             ship->vx = target->vx;
             ship->vy = target->vy;
             ship->vz = target->vz;
          }
       }
    }
    
    ship->energy -= (100+100*ship->shipclass);
         
    ship->vx += (ship->hx*ship->currspeed*2);
    ship->vy += (ship->hy*ship->currspeed*2);
    ship->vz += (ship->hz*ship->currspeed*2);

    echo_to_room( AT_GREEN , get_room_index(ship->pilotseat) , "Launch complete.\n");	
    echo_to_ship( AT_YELLOW , ship , "The ship leaves the platform far behind as it flies into space." );
	// 2/18/04 - Johnson - Modified call to reflect origin of object entering the system
	// 2/19/04 - Johnson - TODO: Add code to reflect where in the system the ship launched from, e.g., a planet or hangar
    //SPRINTF( buf ,"%s enters the starsystem at %.0f %.0f %.0f" , ship->name, ship->vx, ship->vy, ship->vz );
	  SHIP_DATA* fromLocation = ship_from_hanger( ship->game, ship->lastdoc );	// Get their last doc location (where they just launched from)

	if ( fromLocation != 0 )	// Make sure we dont use it if it's null for some reason
	{
		buf = str_printf("%s launches into the starsystem from %s at %.0f %.0f %.0f" , ship->name, fromLocation->name, ship->vx, ship->vy, ship->vz );
	}
	else
	{
		// fromLocation was null for somme reason, use the default
		buf = str_printf("%s launches into the starsystem at %.0f %.0f %.0f" , ship->name, ship->vx, ship->vy, ship->vz );
	} // End changes for Johnson 2/19/04
    echo_to_system( AT_YELLOW, ship, buf , NULL ); 
    echo_to_room( AT_YELLOW , get_room_index(ship->lastdoc) , str_printf("%s lifts off into space.", ship->name) );
                 
}

void do_land( CHAR_DATA *ch, char *argument )
{

    int chance;
    std::string buf;
    SPACE_DATA *spaceobj;
    bool found = FALSE;
    
    scmd_data ctx;
    ctx.rest = argument;
    ctx.arg1 = ctx.rest;

    if (!space_require_ship_from_cockpit( ch , ctx )) return;
    if (!space_require_ship_from_pilotseat( ch , ctx )) return;
    if (!space_require_ship_movement( ch , ctx )) return;
    if (!space_require_ship_tractoree_ed( ch , ctx )) return;    
    if (!space_require_ship_not_docked( ch , ctx )) return;
    if (!space_require_ship_spaceship( ch , ctx )) return;
    if (!space_require_ship_ownership( ch , ctx )) return;

    if (!space_require_tractor_weaker( ch , ctx )) return;
    if (!space_require_ship_autopilot_off( ch , ctx )) return;

    if (!space_require_ship_not_disabled( ch , ctx )) return;
    if (!space_require_ship_not_landed_or_docked( ch , ctx )) return;
    if (!space_require_ship_not_hyperspace( ch , ctx )) return;
    if (!space_require_ship_ready( ch , ctx )) return;
    if (!space_require_ship_energy( ch , ctx , 25 + 25*ctx.ship->shipclass )) return;

    if (!space_require_ship_not_capital( ch , ctx , false) )
    {
        send_to_char("&RCapital ships are to big to land. You'll have to take a shuttle.\n",ch);
        return;
    }

    if ( ctx.arg1.empty() )
    {
        set_char_color(  AT_CYAN, ch );
        ch_printf(ch, "%s" , "Land where?\n\n)");
        space_show_landing_choices( ch , ctx.ship );
        ch_printf(ch, "\nYour Coordinates: %.0f %.0f %.0f\n" ,
                    ctx.ship->vx , ctx.ship->vy, ctx.ship->vz);
          return;
    } 

    for( spaceobj = first_spaceobject; spaceobj; spaceobj = spaceobj->next )
      if( space_in_range( ctx.ship, spaceobj ) )
          if ( !str_prefix(ctx.arg1,spaceobj->locationa) ||
                !str_prefix(ctx.arg1,spaceobj->locationb) ||
                !str_prefix(ctx.arg1,spaceobj->locationc))
          {
            found = TRUE;
            break;
          }

    if( !found )
    {
        ctx.target = get_ship_here( ch->game, ctx.arg1 , ctx.ship );
        if ( !space_require_target_hangar_valid( ch , ctx ) )
            return;
        if ( ship_target_distance( ctx.ship, ctx.target ) > 200 )            
        {
            send_to_char("&R That ship is too far away! You'll have to fly a little closer.\n",ch);
            return;
        }            
    }
    else
    {
        if (!space_in_range( ctx.ship, spaceobj, 500 ) )
        {
            send_to_char("&R That platform is too far away! You'll have to fly a little closer.\n",ch);
            return;
        }                    
    }

    chance = space_chance_by_shipclass( ch, ctx.ship );
    if ( number_percent( ) < chance )
    {
        if ( found )
            ctx.ship->spaceobject = spaceobj;

        set_char_color( AT_GREEN, ch );
        send_to_char( "Landing sequence initiated.\n", ch);
        act( AT_PLAIN, "$n begins the landing sequence.", ch,
        NULL, ctx.arg1 , TO_ROOM );
        buf =  str_printf("%s begins its landing sequence." , ctx.ship->name );
        echo_to_system( AT_YELLOW, ctx.ship, buf , NULL );
        echo_to_docked( AT_YELLOW , ctx.ship, "The ship begins to enter the atmosphere." );

        echo_to_ship( AT_YELLOW , ctx.ship , "The ship slowly begins its landing approach.");
        if (ctx.ship->dest)
            STRFREE(ctx.ship->dest);
        ctx.ship->dest = STRALLOC(ctx.arg1);
        space_set_shipstate(ctx.ship, SHIP_LAND);
        
        ctx.ship->goalspeed = 0;
        ctx.ship->accel = get_acceleration(ctx.ship);
        
        space_learn_from_success( ch, ctx.ship );  
        if ( spaceobject_from_vnum(ctx.ship->game, ctx.ship->lastdoc) != ctx.ship->spaceobject )
        {   
          /*int xp =  (exp_level( ch->skill_level[PILOTING_ABILITY]+1) - exp_level( ch->skill_level[PILOTING_ABILITY])) ;
            xp = UMIN( get_ship_value( ctx.ship ) , xp );
            gain_exp( ch , xp , PILOTING_ABILITY );
            ch_printf( ch, "&WYou gain %ld points of flight experience!\n", UMIN( get_ship_value( ctx.ship ) , xp ) );
            */
            ctx.ship->ch = ch;                      
        }
              return;
    }
    send_to_char("You fail to work the controls properly.\n",ch);
    space_learn_from_failure( ch, ctx.ship );
    return;	
}

void approachland( SHIP_DATA *ship, const std::string& arg)
{
    SPACE_DATA *spaceobj;
    std::string buf2;
    bool found = FALSE;
    SHIP_DATA *target;
    
    for( spaceobj = first_spaceobject; spaceobj; spaceobj = spaceobj->next )
      if( space_in_range( ship, spaceobj ) )
	if ( !str_prefix(arg,spaceobj->locationa) ||
    	     !str_prefix(arg,spaceobj->locationb) ||
    	     !str_prefix(arg,spaceobj->locationc))
    	{
     	    found = TRUE;
     	    break;
     	}

    if( found )
    {
        if ( !str_prefix(arg, spaceobj->locationa) )
        buf2 = spaceobj->locationa;
        else if ( !str_prefix(arg, spaceobj->locationb) )
        buf2 = spaceobj->locationb;
        else if ( !str_prefix(arg, spaceobj->locationc) )
        buf2 = spaceobj->locationc;
    }
    
    target = get_ship_here( ship->game, arg , ship );
    bool target_landing_valid = ( target != ship && target != NULL && target->bayopen
            && ( ship->shipclass != MIDSIZE_SHIP || target->shipclass != MIDSIZE_SHIP ) );

    if ( target_landing_valid )
        buf2 = target->name;

    if ( !found && !target_landing_valid )
    {
        echo_to_room( AT_YELLOW , get_room_index(ship->pilotseat), "Approach failed. Landing aborted.");
        if ( ship->shipstate != SHIP_DISABLED )
            space_set_shipstate( ship, SHIP_READY );
        if (ship->dest)
        {
            STRFREE(ship->dest);
            ship->dest = NULL;
        }
        return;
    }

    echo_to_room( AT_YELLOW , get_room_index(ship->pilotseat), str_printf("Approaching %s.", buf2) );
    echo_to_system( AT_YELLOW, ship, str_printf("%s begins its approach to %s.", ship->name, buf2) , NULL );

    return;
}
    
void landship( SHIP_DATA *ship, const std::string& arg )
{    
    SHIP_DATA *target;
    int destination = 0;
    CHAR_DATA *ch;

    target = get_ship_here( ship->game, arg , ship );
    bool target_landing_valid = ( target != ship && target != NULL && target->bayopen
            && ( ship->shipclass != MIDSIZE_SHIP || target->shipclass != MIDSIZE_SHIP ) );

    if ( target_landing_valid )
    {
        destination = target->hanger;
    }
    else if ( ship->spaceobject )
    {
        if ( !str_prefix(arg,ship->spaceobject->locationa) )
        destination = ship->spaceobject->doca;
        if ( !str_prefix(arg,ship->spaceobject->locationb) )
        destination = ship->spaceobject->docb;
        if ( !str_prefix(arg,ship->spaceobject->locationc) )
        destination = ship->spaceobject->docc;
    }
     
    if ( destination <= 0 || !ship_to_room( ship , destination ) )
    {
        echo_to_room( AT_YELLOW , get_room_index(ship->pilotseat), "Could not complete approach. Landing aborted.");
        echo_to_ship( AT_YELLOW , ship , "The ship pulls back up out of its landing sequence.");
        if (ship->shipstate != SHIP_DISABLED)
            space_set_shipstate( ship, SHIP_READY );
        if (ship->dest)
        {
            STRFREE(ship->dest);
            ship->dest = NULL;
        }           
        return;
    }      
         
    echo_to_room( AT_YELLOW , get_room_index(ship->pilotseat), "Landing sequence complete.");
    echo_to_ship( AT_YELLOW , ship , "You feel a slight thud as the ship sets down on the ground."); 
    echo_to_system( AT_YELLOW, ship, str_printf("%s disappears from your scanner." , ship->name) , NULL );

    if( ship->ch && ship->ch->desc )
    {
        int xp;
        
        ch = ship->ch;
        xp =  (exp_level( ch->skill_level[PILOTING_ABILITY]+1) - exp_level( ch->skill_level[PILOTING_ABILITY])) ;
        xp = UMIN( get_ship_value( ship ) , xp );
        gain_exp( ch , xp , PILOTING_ABILITY );
        ch_printf( ch, "&WYou gain %ld points of flight experience!\n", UMIN( get_ship_value( ship ) , xp ) );
        ship->ch = NULL;
    }

    ship->location = destination;
    ship->lastdoc = ship->location;
    if (ship->shipstate != SHIP_DISABLED)
       ship->shipstate = SHIP_LANDED;
    if (ship->dest)
    {
        STRFREE(ship->dest);
        ship->dest = NULL;
    }     
    ship_from_spaceobject(ship, ship->spaceobject);
    if (ship->tractored)
    {
      if (ship->tractored->tractoredby == ship)
        ship->tractored->tractoredby = NULL;
      ship->tractored = NULL;
    }
    
    echo_to_room( AT_YELLOW , get_room_index(ship->location) , str_printf("%s lands on the platform.", ship->name) );
    
    ship->energy = ship->energy - 25 - 25*ship->shipclass;
    
    if ( !str_cmp("Public",ship->owner) || !str_cmp("trainer",ship->owner) || ship->shipclass == SHIP_TRAINER )
    {
       ship->energy = ship->mod->maxenergy;
       ship->shield = 0;
       ship->autorecharge = FALSE;
       ship->autotrack = FALSE;
       ship->autospeed = FALSE;
       ship->hull = ship->mod->maxhull;
       
       ship->missilestate = MISSILE_READY;
       ship->statet0 = LASER_READY;
       ship->shipstate = SHIP_LANDED;
       
       echo_to_cockpit( AT_YELLOW , ship , "Repairing and refueling ship..." );
    }
    
       save_ship(ship);   
}

void do_accelerate( CHAR_DATA *ch, char *argument )
{
    scmd_data ctx;
    ctx.rest = argument;

    if (!space_require_ship_from_cockpit( ch , ctx )) return;
    if (!space_require_ship_from_pilotseat( ch , ctx )) return;
    if (!space_require_ship_spaceship( ch , ctx )) return;
    if (!space_require_ship_movement( ch , ctx )) return;
    if (!space_require_ship_tractoree_ed( ch , ctx )) return;    
    if (!space_require_ship_autopilot_off( ch , ctx )) return;
    if (!space_require_ship_not_hyperspace( ch , ctx )) return;
    if (!space_require_ship_not_disabled( ch , ctx )) return;
    if (!space_require_ship_not_landed( ch , ctx )) return;

    std::string buf;
    int chance;
    int change;
    int accel;

    int energy_needed = abs((strtoi(ctx.arg1)-abs(ctx.ship->currspeed))/10);
    if (!space_require_ship_energy( ch , ctx , energy_needed )) return;

    ctx.rest = one_argument( ctx.rest, ctx.arg1 );
    
    if( ctx.rest.empty() || !str_cmp(ctx.rest, "max") )
      accel = get_acceleration( ctx.ship );
    else
      accel = strtoi( ctx.rest );
      
    accel = URANGE( 0, accel, get_acceleration( ctx.ship ) );
      
    ctx.ship->accel = accel;

    chance = space_chance_by_shipclass( ch, ctx.ship );

    if ( number_percent( ) >= chance )
    {
        send_to_char("&RYou fail to work the controls properly.\n",ch);
        space_learn_from_failure( ch, ctx.ship );
        return;	
    }
                
    change = strtoi(ctx.arg1);
                      
    act( AT_PLAIN, "$n manipulates the ships controls.", ch,
    NULL, ctx.rest , TO_ROOM );

    ctx.ship->goalspeed = URANGE( 0 , change , ctx.ship->mod->realspeed );

    if ( change > ctx.ship->currspeed )
    {
       ctx.ship->inorbitof = NULL;
       send_to_char( "&GAccelerating\n", ch);
       echo_to_cockpit( AT_YELLOW , ctx.ship , "The ship begins to accelerate.");
       echo_to_docked( AT_YELLOW , ctx.ship, "The hull groans at an increase in speed." );
       buf = std::string(ctx.ship->name) + " begins to speed up.";
       echo_to_system( AT_ORANGE , ctx.ship , buf , NULL );
       ctx.ship->currspeed = URANGE( 0, (ctx.ship->currspeed+accel), ctx.ship->goalspeed );

    }
    
    if ( change < ctx.ship->currspeed )
    {
       send_to_char( "&GDecelerating\n", ch);
       echo_to_cockpit( AT_YELLOW , ctx.ship , "The ship begins to slow down.");
       echo_to_docked( AT_YELLOW , ctx.ship, "The hull groans as the ship slows." );
       buf = std::string(ctx.ship->name) + " begins to slow down.";
       echo_to_system( AT_ORANGE , ctx.ship , buf , NULL );
       ctx.ship->currspeed = URANGE( ctx.ship->goalspeed, (ctx.ship->currspeed-ctx.ship->accel), ctx.ship->currspeed );
    }
    		     
    ctx.ship->energy -= energy_needed;
    
    ctx.ship->goalspeed = URANGE( 0 , change , ctx.ship->mod->realspeed );
         
    space_learn_from_success( ch, ctx.ship );
    	

}

void do_trajectory_actual( CHAR_DATA *ch, char *argument )
{
    scmd_data ctx;
    ctx.rest = argument;

    if (!space_require_ship_from_cockpit( ch , ctx )) return;
    if (!space_require_ship_spaceship( ch , ctx )) return;
    if (!space_require_ship_movement( ch , ctx )) return;
    if (!space_require_ship_from_pilotseat( ch , ctx )) return;
    if (!space_require_ship_autopilot_off( ch , ctx )) return;
    if (!space_require_ship_not_disabled( ch , ctx )) return;
    if (!space_require_ship_not_hyperspace( ch , ctx )) return;
    if (!space_require_ship_not_landed( ch , ctx )) return;
    if (!space_require_ship_not_disabled( ch , ctx )) return;
    if (!space_require_ship_ready( ch , ctx )) return;


    std::string  buf;
    int chance;
    float vx,vy,vz;
    
    int required_energy = (ctx.ship->currspeed/10);

    if (!space_require_ship_energy( ch , ctx , required_energy )) return;   

    chance = space_chance_by_shipclass( ch, ctx.ship );
    if ( number_percent( ) > chance )
    		{ 
	        send_to_char("&RYou fail to work the controls properly.\n",ch);
          space_learn_from_failure( ch, ctx.ship );
          return;	
    }
    	
    ctx.rest = one_argument( ctx.rest, ctx.arg2 );
    ctx.rest = one_argument( ctx.rest, ctx.arg3 );
            
    vx = strtof( ctx.arg2.c_str(), NULL );
    vy = strtof( ctx.arg3.c_str(), NULL );
    vz = strtof( ctx.rest.c_str(), NULL );
            
    if ( vx == ctx.ship->vx && vy == ctx.ship->vy && vz == ctx.ship->vz )
    {
       ch_printf( ch , "The ship is already at %.0f %.0f %.0f !" ,vx,vy,vz);
    }

    if( vx == 0 && vy == 0 && vz == 0 )
    	vz = ctx.ship->vz+1;
                
    ctx.ship->hx = vx - ctx.ship->vx;
    ctx.ship->hy = vy - ctx.ship->vy;
    ctx.ship->hz = vz - ctx.ship->vz;
    
    ctx.ship->energy -= required_energy;
       
    ch_printf( ch ,"&GNew course set, approaching %.0f %.0f %.0f.\n" , vx,vy,vz );            
    act( AT_PLAIN, "$n manipulates the ships controls.", ch, NULL, ctx.rest , TO_ROOM );
                         
    echo_to_cockpit( AT_YELLOW ,ctx.ship, "The ship begins to turn.\n" );                        
    buf = std::string(ctx.ship->name) + " turns altering its present course.";
    echo_to_system( AT_ORANGE , ctx.ship , buf , NULL );
                                                            
    if ( ctx.ship->shipclass == FIGHTER_SHIP || ( ctx.ship->shipclass == MIDSIZE_SHIP && ctx.ship->mod->manuever > 50 ) )
        ctx.ship->shipstate = SHIP_BUSY_3;
    else if ( ctx.ship->shipclass == MIDSIZE_SHIP || ( ctx.ship->shipclass == CAPITAL_SHIP && ctx.ship->mod->manuever > 50 ) )
        ctx.ship->shipstate = SHIP_BUSY_2;
    else
        ctx.ship->shipstate = SHIP_BUSY;     
   
    space_learn_from_success( ch, ctx.ship );    	
}

void do_trajectory( CHAR_DATA *ch, char *argument )
{
    scmd_data ctx;
    ctx.rest = argument;

    if (!space_require_ship_from_cockpit( ch , ctx )) return;
    if (!space_require_ship_spaceship( ch , ctx )) return;
    if (!space_require_ship_movement( ch , ctx )) return;
    if (!space_require_ship_from_pilotseat( ch , ctx )) return;
    if (!space_require_ship_autopilot_off( ch , ctx )) return;
    if (!space_require_ship_not_disabled( ch , ctx )) return;
    if (!space_require_ship_not_hyperspace( ch , ctx )) return;
    if (!space_require_ship_not_landed( ch , ctx )) return;
    if (!space_require_ship_not_disabled( ch , ctx )) return;
    if (!space_require_ship_ready( ch , ctx )) return;

    int required_energy = (ctx.ship->currspeed/10);
    std::string buf;
    int chance;
    float vx,vy,vz;
    
    if (!space_require_ship_energy( ch , ctx , required_energy )) return;  
    
    chance = space_chance_by_shipclass( ch, ctx.ship );
    if ( number_percent( ) > chance )
    { 
        send_to_char("&RYou fail to work the controls properly.\n",ch);
        space_learn_from_failure( ch, ctx.ship );
        return;	
    }
    	
    ctx.rest = one_argument( ctx.rest, ctx.arg2 );
    ctx.rest = one_argument( ctx.rest, ctx.arg3 );

    if( !ctx.arg2.empty() )
      vx = atof( ctx.arg2.c_str() );
    else
      vx = 0;

    if( !ctx.arg3.empty() )
      vy = atof( ctx.arg3.c_str() );
    else
      vy = 0;

    if( !ctx.rest.empty() )
      vz = atof( ctx.rest.c_str() );
    else if ( vx != ctx.ship->vx && vy != ctx.ship->vy )
      vz = 0;
    else
      vz = 1;
            
    ctx.ship->hx = vx;
    ctx.ship->hy = vy;
    ctx.ship->hz = vz;
    
    ctx.ship->energy -= (required_energy);
       
    ch_printf( ch ,"&GNew course set, approaching %.0f %.0f %.0f.\n" , vx,vy,vz );            
    act( AT_PLAIN, "$n manipulates the ships controls.", ch, NULL, ctx.rest , TO_ROOM );
                         
    echo_to_cockpit( AT_YELLOW ,ctx.ship, "The ship begins to turn.\n" );                        
    buf = std::string(ctx.ship->name) + " turns altering its present course.";
    echo_to_system( AT_ORANGE , ctx.ship , buf , NULL );
                                                            
    if ( ctx.ship->shipclass == FIGHTER_SHIP || ( ctx.ship->shipclass == MIDSIZE_SHIP && ctx.ship->mod->manuever > 50 ) )
        ctx.ship->shipstate = SHIP_BUSY_3;
    else if ( ctx.ship->shipclass == MIDSIZE_SHIP || ( ctx.ship->shipclass == CAPITAL_SHIP && ctx.ship->mod->manuever > 50 ) )
        ctx.ship->shipstate = SHIP_BUSY_2;
    else
        ctx.ship->shipstate = SHIP_BUSY;     
   
    space_learn_from_success( ch, ctx.ship );     	
}

static bool can_buy_ship_type( CHAR_DATA *ch, CLAN_DATA *clan, SHIP_DATA *ship, ShipBuyContext ctx )
{
    CLAN_DATA *mainclan = get_mainclan( clan );

    if ( ship->type == SHIP_IMPERIAL )
    {
        if ( !mainclan || str_cmp( mainclan->name, "The Empire" ) )
        {
            send_to_char( "&RThat ship may only be purchaced by the Empire!\n", ch );
            return false;
        }
    }

    if ( ship->type == SHIP_REBEL )
    {
        if ( !mainclan
        || ( str_cmp( mainclan->name, "The Rebel Alliance" )
          && str_cmp( mainclan->name, "The New Republic" ) ) )
        {
            send_to_char( "&RThat ship may only be purchaced by The Rebel Alliance!\n", ch );
            return false;
        }
    }

    if ( ctx == ShipBuyContext::CLAN )
    {
        if ( clan && !str_cmp( clan->name, "The Empire" ) && ship->type != SHIP_IMPERIAL )
        {
            send_to_char( "&RDue to contractual agreements that ship may not be purchased by the empire!\n", ch );
            return false;
        }
    }

    return true;
}

static bool buy_ship_common( CHAR_DATA *ch,
                             SHIP_DATA *ship,
                             const char *argument,
                             const char *buyer_name,
                             ShipBuyContext ctx,
                             CLAN_DATA *clan = NULL )
{
    long price  = get_ship_value( ship );

    if ( ship_is_owned( ship ) )
    {
        send_to_char( "&RThat ship isn't for sale!\n", ch );
        return false;
    }

    if ( !can_buy_ship_type( ch, clan, ship, ctx ) )
        return false;

    if ( ctx == ShipBuyContext::PLAYER )
    {
        if ( ch->gold < price )
        {
            ch_printf( ch, "&RThis ship costs %ld. You don't have enough credits!\n", price );
            return false;
        }

        ch->gold -= static_cast<int>( price );
        ch_printf( ch, "&GYou pay %ld credits to purchase the ship.\n", price );
    }
    else if ( ctx == ShipBuyContext::CLAN )
    {
        if ( !clan )
            return false;

        if ( clan->funds < price )
        {
            ch_printf( ch, "&RThis ship costs %ld. You don't have enough credits!\n", price );
            return false;
        }

        clan->funds -= price;
        ch_printf( ch, "&G%s pays %ld credits to purchase the ship.\n", buyer_name, price );
    }

    act( AT_PLAIN,"$n walks over to a terminal and makes a credit transaction.",
         ch, NULL, argument, TO_ROOM );
    STRFREE( ship->owner );
    ship->owner = STRALLOC( buyer_name );
    save_ship( ship );         

    if ( ctx == ShipBuyContext::CLAN && clan )
    {
        if ( ship->shipclass <= SHIP_PLATFORM )
            clan->spacecraft++;
        else
            clan->vehicles++;
    }

    return true;
}

void do_buyship( CHAR_DATA *ch, char *argument )
{
    SHIP_DATA *ship;

    if ( IS_NPC( ch ) || !ch->pcdata )
    {
        send_to_char( "&ROnly players can do that!\n", ch );
        return;
    }

    ship = find_ship_in_or_room( ch, argument, true );
    if ( !ship )
        return;

    buy_ship_common( ch,
                     ship,
                     argument,
                     ch->name,
                     ShipBuyContext::PLAYER,
                     ch->pcdata->clan );
}

void do_clanbuyship( CHAR_DATA *ch, char *argument )
{
    SHIP_DATA *ship;
    CLAN_DATA *clan;

    if ( IS_NPC( ch ) || !ch->pcdata )
    {
        send_to_char( "&ROnly players can do that!\n", ch );
        return;
    }

    if ( !ch->pcdata->clan )
    {
        send_to_char( "&RYou aren't a member of any organizations!\n", ch );
        return;
    }

    clan = ch->pcdata->clan;

    if ( !( ( ch->pcdata->bestowments && is_name( "clanbuyship", ch->pcdata->bestowments ) )
        || !str_cmp( ch->name, clan->leader ) ) )
    {
        send_to_char( "&RYour organization hasn't seen fit to bestow you with that ability.\n", ch );
        return;
    }

    ship = find_ship_in_or_room( ch, argument, true );
    if ( !ship )
        return;

    buy_ship_common( ch,
                     ship,
                     argument,
                     clan->name,
                     ShipBuyContext::CLAN,
                     clan );
}

static bool sell_ship_common( CHAR_DATA *ch,
                              SHIP_DATA *ship,
                              const char *argument,
                              const char *seller_name,
                              ShipBuyContext ctx,
                              CLAN_DATA *clan = NULL )
{
    long price;
    long payout;

    if ( !ship_is_owned( ship ) )
    {
        send_to_char( "&RThat ship is not owned!\n", ch );
        return false;
    }

    if ( str_cmp( ship->owner, seller_name ) )
    {
        send_to_char( "&RThat isn't your ship!\n", ch );
        return false;
    }

    price = get_ship_value( ship );
    payout = price - price / 10;

    if ( ctx == ShipBuyContext::PLAYER )
    {
        ch->gold += static_cast<int>( payout );
        ch_printf( ch, "&GYou receive %ld credits from selling your ship.\n", payout );
    }
    else if ( ctx == ShipBuyContext::CLAN )
    {
        if ( !clan )
            return false;

        clan->funds += payout;
        ch_printf( ch, "&GYour clan receives %ld credits from selling your ship.\n", payout );
    }

    act( AT_PLAIN,
         "$n walks over to a terminal and makes a credit transaction.",
         ch, NULL, argument, TO_ROOM );

    STRFREE( ship->owner );
    ship->owner = STRALLOC( "" );
    STRFREE( ship->pilot );
    ship->pilot = STRALLOC( "" );
    STRFREE( ship->copilot );
    ship->copilot = STRALLOC( "" );

    save_ship( ship );

    return true;
}

void do_sellship( CHAR_DATA *ch, char *argument )
{
    SHIP_DATA *ship;

    ship = find_ship_in_or_room( ch, argument, true );
    if ( !ship )
        return;

    sell_ship_common( ch,
                      ship,
                      argument,
                      ch->name,
                      ShipBuyContext::PLAYER );
}

void do_clansellship( CHAR_DATA *ch, char *argument )
{
    SHIP_DATA *ship;
    CLAN_DATA *clan;

    if ( IS_NPC( ch ) || !ch->pcdata )
    {
        send_to_char( "&ROnly players can do that!\n", ch );
        return;
    }

    if ( !ch->pcdata->clan )
    {
        send_to_char( "&RYou aren't a member of any organizations!\n", ch );
        return;
    }

    clan = ch->pcdata->clan;

    if ( !( ( ch->pcdata->bestowments && is_name( "clanbuyship", ch->pcdata->bestowments ) )
         || !str_cmp( ch->name, clan->leader ) ) )
    {
        send_to_char( "&RYour organization hasn't seen fit to bestow you with that ability.\n", ch );
        return;
    }

    ship = find_ship_in_or_room( ch, argument, true );
    if ( !ship )
        return;

    sell_ship_common( ch,
                      ship,
                      argument,
                      clan->name,
                      ShipBuyContext::CLAN,
                      clan );
}

void do_info(CHAR_DATA *ch, char *argument )
{
    SHIP_DATA *ship;
    SHIP_DATA *target;
    bool fromafar = TRUE;
    
    if (  (ship = ship_from_cockpit(ch->game, ch->in_room->vnum))  == NULL )
    {
        if ( argument[0] == '\0' )
        {
            act( AT_PLAIN, "Which ship do you want info on?.", ch, NULL, NULL, TO_CHAR );
            return;
        }

        ship = ship_in_room( ch->in_room , argument );
        if ( !ship )
        {
            act( AT_PLAIN, "I see no $T here.", ch, NULL, argument, TO_CHAR );
            return;
        }
        target = ship;
    }
    else if ( ship->hanger == ch->in_room->vnum )
    {
        if ( argument[0] == '\0' )
        {
            act( AT_PLAIN, "Which ship do you want info on?.", ch, NULL, NULL, TO_CHAR );
            return;
        }

        ship = ship_in_room( ch->in_room , argument );
        if ( !ship )
        {
            act( AT_PLAIN, "I see no $T here.", ch, NULL, argument, TO_CHAR );
            return;
        }

        target = ship;
    }

    else if (argument[0] == '\0')
    {
        target = ship;
        fromafar = FALSE;
    }
    else
        target = get_ship_here( ship->game, argument , ship );

    if ( target == NULL )
    {
        send_to_char("&RI don't see that here.\nTry the radar, or type info by itself for info on this ship.\n",ch);
        return;
    }

    if ( check_pilot( ch , target ) )
        fromafar = FALSE;

    if ( !space_distance_ship_less_than( ship, target, 500+ship->mod->sensor*2 ) )
    {
        send_to_char("&RThat ship is to far away to scan.\n",ch);
        return;
  }

    ship_show( ch, target, fromafar );

    act( AT_PLAIN, "$n checks various gages and displays on the control panel.", ch,
        NULL, argument , TO_ROOM );
	  
}

void do_autorecharge(CHAR_DATA *ch, char *argument )
{
    scmd_data ctx;
    ctx.rest = argument;

    if (!space_require_ship_from_cockpit( ch , ctx )) return;
    if (!space_require_ship_from_coseat( ch , ctx )) return;
    if (!space_require_ship_autopilot_off( ch , ctx )) return;

    int chance;
    int recharge;

    chance = space_chance_by_shipsystems( ch, ctx.ship );
    if ( number_percent( ) > chance )
    {
        send_to_char("&RYou fail to work the controls properly.\n",ch);
        learn_from_failure( ch, gsn_shipsystems );
        return;	
    }
    
    act( AT_PLAIN, "$n flips a switch on the control panel.", ch,
         NULL, argument , TO_ROOM );

    if ( !str_cmp(argument,"on" ) )
    {
        ctx.ship->autorecharge=TRUE;
        send_to_char( "&GYou power up the shields.\n", ch);
        echo_to_cockpit( AT_YELLOW , ctx.ship , "Shields ON. Autorecharge ON.");
    }
    else if ( !str_cmp(argument,"off" ) )
    {
        ctx.ship->autorecharge=FALSE;
        send_to_char( "&GYou shutdown the shields.\n", ch);
        echo_to_cockpit( AT_YELLOW , ctx.ship , "Shields OFF. Shield strength set to 0. Autorecharge OFF.");
        ctx.ship->shield = 0;
    }
    else if ( !str_cmp(argument,"idle" ) )
    {
        ctx.ship->autorecharge=FALSE;
        send_to_char( "&GYou let the shields idle.\n", ch);
        echo_to_cockpit( AT_YELLOW , ctx.ship , "Autorecharge OFF. Shields IDLING.");
    }
    else
    {   
        if (ctx.ship->autorecharge == TRUE)
        {
           ctx.ship->autorecharge=FALSE;
           send_to_char( "&GYou toggle the shields.\n", ch);
           echo_to_cockpit( AT_YELLOW , ctx.ship , "Autorecharge OFF. Shields IDLING.");
        }
        else
        {
           ctx.ship->autorecharge=TRUE;
           send_to_char( "&GYou toggle the shields.\n", ch);
           echo_to_cockpit( AT_YELLOW , ctx.ship , "Shields ON. Autorecharge ON.");
        }   
    }
    
    if (ctx.ship->autorecharge)
    {
       recharge  = URANGE( 1, ctx.ship->mod->maxshield-ctx.ship->shield, 25+ctx.ship->shipclass*25 );
       recharge  = UMIN( recharge, ctx.ship->energy*5 + 100 );
       ctx.ship->shield += recharge;
       ctx.ship->energy -= ( recharge*2 + recharge * ctx.ship->shipclass );
    }
                                          	  
    learn_from_success( ch, gsn_shipsystems );    	
}

void do_autopilot(CHAR_DATA *ch, char *argument )
{
    scmd_data ctx;
    ctx.rest = argument;

    if (!space_require_ship_from_cockpit( ch , ctx )) return;
    if (!space_require_ship_from_pilotseat( ch , ctx )) return;
    if (!space_require_ship_ownership( ch , ctx )) return;
    space_validate_ship_links( ch, ctx.ship, false );
    if ( ctx.ship->shipstate == SHIP_DOCKED )
    {
        if(ctx.ship->docked == NULL || ( ctx.ship->docked->shipclass > MIDSIZE_SHIP && ctx.ship->shipclass > MIDSIZE_SHIP ))
        {
            send_to_char("&RNot until after you've launched!\n",ch);
            return;
        }
        send_to_char("&RNot while you are docked!\n",ch);
        return;
    }

    if ( ctx.ship->target0 )
    {
        ctx.ship->autotrack = FALSE;
    }
  
        
    act( AT_PLAIN, "$n flips a switch on the control panel.", ch,
         NULL, argument , TO_ROOM );

    if ( ( ctx.ship->autopilot == TRUE && str_cmp(argument,"on") )
    || !str_cmp(argument,"off") )
    {
        ctx.ship->autopilot=FALSE;
        send_to_char( "&GYou toggle the autopilot.\n", ch);
        echo_to_cockpit( AT_YELLOW , ctx.ship , "Autopilot OFF.");
    }
    else
    {
        if( ctx.ship->shipstate == SHIP_LANDED )
        {
          send_to_char("&RNot while you are docked!\n",ch);
          return;
        }
        ctx.ship->autopilot=TRUE;
        ctx.ship->autorecharge = TRUE;
        send_to_char( "&GYou toggle the autopilot.\n", ch);
        echo_to_cockpit( AT_YELLOW , ctx.ship , "Autopilot ON.");
    }   

}

void do_openhatch(CHAR_DATA *ch, const std::string& argument )
{
    scmd_data ctx;  
    ctx.rest = argument;

    if ( ctx.rest.empty() || !str_cmp(ctx.rest,"hatch") )
    {
        ctx.ship = ship_from_entrance( ch->game, ch->in_room->vnum );
        if( ctx.ship == NULL)
        {
            send_to_char( "&ROpen what?\n", ch );
            return;
        }
        else
        {
          if ( !ctx.ship->hatchopen)
       	  {

              if  ( ctx.ship->shipclass == SHIP_PLATFORM )
              {
                  send_to_char( "&RTry one of the docking bays!\n" , ch );
                  return;
              }
              if ( ctx.ship->location != ctx.ship->lastdoc ||
                ( ctx.ship->shipstate != SHIP_LANDED && ctx.ship->shipstate != SHIP_DISABLED ) )
              {
                send_to_char("&RPlease wait till the ship lands!\n",ch);
                return;
              }
              ctx.ship->hatchopen = TRUE;
              send_to_char("&GYou open the hatch.\n",ch);
              act( AT_PLAIN, "$n opens the hatch.", ch, NULL, ctx.rest , TO_ROOM );
              echo_to_room( AT_YELLOW , get_room_index(ctx.ship->location) , str_printf("The hatch on %s opens." , ctx.ship->name) );
              sound_to_room( get_room_index(ctx.ship->entrance) , "!!SOUND(door)" );
              sound_to_room( get_room_index(ctx.ship->location) , "!!SOUND(door)" );
              return;
       	  }
       	  else
       	  {
       	     send_to_char("&RIt's already open.\n",ch);
       	     return;
       	  }
        }
    }

    ctx.ship = ship_in_room( ch->in_room , ctx.rest );
    if ( !ctx.ship )
    {
        act( AT_PLAIN, "I see no $T here.", ch, NULL, ctx.rest, TO_CHAR );
        return;
    }

    if ( ctx.ship->shipstate != SHIP_LANDED && ctx.ship->shipstate != SHIP_DISABLED )
    {
        send_to_char( "&RThat ship has already started to launch",ch);
        return;
    }
    
    if (!space_require_ship_ownership( ch , ctx )) return;
              
    if ( !ctx.ship->hatchopen)
    {
        ctx.ship->hatchopen = TRUE;
        act( AT_PLAIN, "You open the hatch on $T.", ch, NULL, ctx.ship->name, TO_CHAR );
        act( AT_PLAIN, "$n opens the hatch on $T.", ch, NULL, ctx.ship->name, TO_ROOM );
        echo_to_room( AT_YELLOW , get_room_index(ctx.ship->entrance) , "The hatch opens from the outside." );
        sound_to_room( get_room_index(ctx.ship->entrance) , "!!SOUND(door)" );
                sound_to_room( get_room_index(ctx.ship->location) , "!!SOUND(door)" );		
        return;
    }

    send_to_char("&GIts already open!\n",ch);

}

void do_openhatch(CHAR_DATA *ch, char *argument )
{
    do_openhatch(ch, argument ? std::string(argument) : std::string());
}


void do_closehatch(CHAR_DATA *ch, const std::string& argument )
{
    scmd_data ctx;
    ctx.rest = argument;

    if ( ctx.rest.empty() || !str_cmp(ctx.rest,"hatch") )
    {
        ctx.ship = ship_from_entrance( ch->game, ch->in_room->vnum );
        if( ctx.ship == NULL)
        {
            send_to_char( "&RClose what?\n", ch );
            return;
        }
        else
        {
            
            if  ( ctx.ship->shipclass == SHIP_PLATFORM )
            {
              send_to_char( "&RTry one of the docking bays!\n" , ch );
              return;
                  }   
            if ( ctx.ship->hatchopen)
            {
                ctx.ship->hatchopen = FALSE;
                send_to_char("&GYou close the hatch.\n",ch);
                act( AT_PLAIN, "$n closes the hatch.", ch, NULL, ctx.rest, TO_ROOM );  
                echo_to_room( AT_YELLOW , get_room_index(ctx.ship->location) , str_printf("The hatch on %s closes." , ctx.ship->name) );
                sound_to_room( get_room_index(ctx.ship->entrance) , "!!SOUND(door)" );
                sound_to_room( get_room_index(ctx.ship->location) , "!!SOUND(door)" );		
                return;
            }
            else
            {
                send_to_char("&RIt's already closed.\n",ch);
                return;
            }
        }
    }
    
    ctx.ship = ship_in_room( ch->in_room , ctx.rest );
    if ( !ctx.ship )
    {
        act( AT_PLAIN, "I see no $T here.", ch, NULL, ctx.rest, TO_CHAR );
        return;
    }

    if ( ctx.ship->shipstate != SHIP_LANDED && ctx.ship->shipstate != SHIP_DISABLED )
    {
        send_to_char( "&RThat ship has already started to launch",ch);
        return;
    }
    else
    {
        if(ctx.ship->hatchopen)
        {
            ctx.ship->hatchopen = FALSE;
            act( AT_PLAIN, "You close the hatch on $T.", ch, NULL, ctx.ship->name, TO_CHAR );
            act( AT_PLAIN, "$n closes the hatch on $T.", ch, NULL, ctx.ship->name, TO_ROOM );
            echo_to_room( AT_YELLOW , get_room_index(ctx.ship->entrance) , "The hatch is closed from outside.");
            sound_to_room( get_room_index(ctx.ship->entrance) , "!!SOUND(door)" );
            sound_to_room( get_room_index(ctx.ship->location) , "!!SOUND(door)" );		

            return;
        }
        else
        {
            send_to_char("&RIts already closed.\n",ch);
            return;
        }
    }


}

void do_closehatch(CHAR_DATA *ch, char *argument )
{
    do_closehatch(ch, argument ? std::string(argument) : std::string());
}

void do_status(CHAR_DATA *ch, char *argument )
{
    scmd_data ctx;
    ctx.rest = argument;

    int chance;
    SHIP_DATA *ship;
    SHIP_DATA *target;
    
    if (  (ship = ship_from_cockpit(ch->game, ch->in_room->vnum))  == NULL )
    {
        send_to_char("&RYou must be in the cockpit, turret or engineroom of a ship to do that!\n",ch);
        return;
    }
    
    if (ctx.rest.empty())
        target = ship;
    else
        target = get_ship_here( ship->game, ctx.rest , ship );
    
    if ( target == NULL )
    {
        send_to_char("&RI don't see that here.\nTry the radar, or type status by itself for your ships status.\n",ch);
        return;
    }          
    
    if ( !space_distance_ship_less_than( ship, target, 500+ship->mod->sensor*2 ) )
    {
        send_to_char("&RThat ship is to far away to scan.\n",ch);
        return;
    }
    
    chance = space_chance_by_shipsystems( ch, ctx.ship );
    if ( number_percent( ) > chance )
    {
        send_to_char("&RYou cant figure out what the readout means.\n",ch);
        learn_from_failure( ch, gsn_shipsystems );
        return;	
    }
        
    act( AT_PLAIN, "$n checks various gages and displays on the control panel.", ch,
         NULL, ctx.rest , TO_ROOM );
    
    ship_show_status( ch, target );

    learn_from_success( ch, gsn_shipsystems );
}

void do_hyperspace(CHAR_DATA *ch, char *argument )
{
    scmd_data ctx;
    ctx.rest = argument ? std::string(argument) : std::string();

    int chance;
    float tx, ty, tz;
    SHIP_DATA *dship, *target;
    SPACE_DATA *spaceobject;
    std::string buf;
    if (!space_require_ship_from_cockpit( ch , ctx )) return;
    if (!space_require_ship_from_pilotseat( ch , ctx )) return;
    if (!space_require_ship_autopilot_off( ch , ctx )) return;
    if (!space_require_ship_movement( ch , ctx )) return;
    if (!space_require_ship_spaceship( ch , ctx )) return;
   
    if (ctx.ship->mod->hyperspeed == 0)
    {
        send_to_char("&RThis ship is not equipped with a hyperdrive!\n",ch);
        return;
    }
    if (( ctx.rest.empty() || str_cmp( ctx.rest.c_str(), "off" )) && ctx.ship->shipstate == SHIP_HYPERSPACE)
    {
        send_to_char("&RYou are already travelling lightspeed!\n",ch);
        return;
    }
		if ( ctx.ship->mod->gravproj )
		{
		  send_to_char ("&RYou can't enter hyperspace with gravity wells activated!\n", ch);
		  return;
		}

		for( target = first_ship; target; target= target->next )
		  if (target && ctx.ship && target != ctx.ship ) 
		  {
		    if ( target->spaceobject && ctx.ship->spaceobject &&  target->shipstate != SHIP_LANDED && 
		      target->mod && target->mod->gravitypower && target->mod->gravproj &&
		        space_distance_ship_less_than( ctx.ship, target, 10*target->mod->gravproj ) )
		    {
		      ch_printf(ch, "You are still within the %s's artificial gravity well.\n", target->name );
		      return;
		    }
		  }

    if ( !ctx.rest.empty() && (!str_cmp( ctx.rest, "off" ) &&  ctx.ship->shipstate != SHIP_HYPERSPACE) )
    {
        send_to_char("&RHyperdrive not active.\n",ch);
        return;
    }

    if (!space_require_ship_not_disabled( ch , ctx )) return;
    if (!space_require_ship_not_landed( ch , ctx )) return;
    if (!space_require_ship_tractoree_ed( ch , ctx )) return;

    if (ctx.ship->shipstate != SHIP_READY && ctx.ship->shipstate != SHIP_HYPERSPACE)
    {
        send_to_char("&RPlease wait until the ship has finished its current manouver.\n",ch);
        return;
    }

    if ( argument && !str_cmp( argument, "off" ) &&  ctx.ship->shipstate == SHIP_HYPERSPACE)
    {
        ship_to_spaceobject (ctx.ship, ctx.ship->currjump);
        
        if (ctx.ship->spaceobject == NULL)
        {
            echo_to_cockpit( AT_RED, ctx.ship, "Ship lost in Hyperspace. Make new calculations.");
            return;
        }
        else
        {
            //float tx, ty, tz;

            for( spaceobject = first_spaceobject; spaceobject; spaceobject = spaceobject->next )
                if( space_in_range( ctx.ship, spaceobject ) )
                {
                  ctx.ship->currjump = spaceobject;
                  break;
                }
            if( !spaceobject )
                ctx.ship->currjump = ctx.ship->spaceobject;

            tx = ctx.ship->vx;
            ty = ctx.ship->vy;
            tz = ctx.ship->vz;

            ctx.ship->vx = ctx.ship->cx;
            ctx.ship->vy = ctx.ship->cy;
            ctx.ship->vz = ctx.ship->cz;

            ctx.ship->cx = tx;
            ctx.ship->cy = ty;
            ctx.ship->cz = tz;
            ctx.ship->currjump = NULL;

            echo_to_room( AT_YELLOW, get_room_index(ctx.ship->pilotseat), "Hyperjump complete.");
            echo_to_ship( AT_YELLOW, ctx.ship, "The ship lurches slightly as it comes out of hyperspace.");
            // 2/18/04 - Johnson - Modified call to reflect origin of object entering the system
            //SPRINTF( buf ,"%s enters the starsystem at %.0f %.0f %.0f" , ctx.ship->name, ctx.ship->vx, ctx.ship->vy, ctx.ship->vz );
            buf = str_printf("%s enters the starsystem from hyperspace at %.0f %.0f %.0f" , ctx.ship->name, ctx.ship->vx, ctx.ship->vy, ctx.ship->vz );
            echo_to_system( AT_YELLOW, ctx.ship, buf , NULL );
            space_set_shipstate(ctx.ship, SHIP_READY);
            STRFREE( ctx.ship->home );
            ctx.ship->home = STRALLOC( ctx.ship->spaceobject->name );
            ctx.ship->dirty = true;

            for( dship = first_ship; dship; dship = dship->next )
              if ( dship->docked && dship->docked == ctx.ship )
              {
                  echo_to_room( AT_YELLOW, get_room_index(dship->pilotseat), "Hyperjump complete.");
                  echo_to_ship( AT_YELLOW, dship, "The ship lurches slightly as it comes out of hyperspace.");
                  // 2/18/04 - Johnson - Modified call to reflect origin of object entering the system
                  //SPRINTF( buf ,"%s enters the starsystem at %.0f %.0f %.0f" , ctx.ship->name, ctx.ship->vx, ctx.ship->vy, ctx.ship->vz );
                  buf = str_printf("%s enters the starsystem from hyperspace at %.0f %.0f %.0f" , ctx.ship->name, ctx.ship->vx, ctx.ship->vy, ctx.ship->vz );
                  echo_to_system( AT_YELLOW, dship, buf , NULL );
                  STRFREE( dship->home );
                  dship->home = STRALLOC( ctx.ship->home );
                  dship->dirty = true;
              }		      	  


          return;

          }   
    } 
        
			
    if (!ctx.ship->currjump)
    {
        send_to_char("&RYou need to calculate your jump first!\n",ch);
        return;
    } 
    int required_energy = 100;
    if (!space_require_ship_energy( ch , ctx , required_energy )) return;  

    if ( ctx.ship->currspeed <= 0 )
    {
          send_to_char("&RYou need to speed up a little first!\n",ch);
          return;
    }

      for( spaceobject = first_spaceobject; spaceobject; spaceobject = spaceobject->next )
    {
      if( ( abs ( (int) ( spaceobject->xpos - ctx.ship->vx ) ) < 100+(spaceobject->gravity*5) )
      && ( abs ( (int) ( spaceobject->ypos - ctx.ship->vy ) ) < 100+(spaceobject->gravity*5) )
      && ( abs ( (int) ( spaceobject->zpos - ctx.ship->vz ) ) < 100+(spaceobject->gravity*5) ) )
            {
                ch_printf(ch, "&RYou are too close to %s to make the jump to lightspeed.\n", spaceobject->name );
                return;
            }
    }

    chance = space_chance_by_shipclass( ch, ctx.ship );
    if ( number_percent( ) >= chance )
    {
        send_to_char("&RYou can't figure out which lever to use.\n",ch);
        space_learn_from_failure( ch, ctx.ship );
        return;
    }
    buf = str_printf("%s enters hyperspace." , ctx.ship->name );
    echo_to_system( AT_YELLOW, ctx.ship, buf , NULL );

    ctx.ship->lastsystem = ctx.ship->spaceobject;
    ship_from_spaceobject( ctx.ship , ctx.ship->spaceobject );
    space_set_shipstate(ctx.ship, SHIP_HYPERSPACE);

    send_to_char( "&GYou push forward the hyperspeed lever.\n", ch);
    act( AT_PLAIN, "$n pushes a lever forward on the control panel.", ch,
         NULL, argument , TO_ROOM );
    echo_to_ship( AT_YELLOW , ctx.ship , "The ship lurches slightly as it makes the jump to lightspeed." );
    echo_to_cockpit( AT_YELLOW , ctx.ship , "The stars become streaks of light as you enter hyperspace.");
    echo_to_docked( AT_YELLOW , ctx.ship, "The stars become streaks of light as you enter hyperspace." );

    ctx.ship->energy -= 100;

    tx = ctx.ship->vx;
    ty = ctx.ship->vy;
    tz = ctx.ship->vz;

    ctx.ship->vx = ctx.ship->jx;
    ctx.ship->vy = ctx.ship->jy;
    ctx.ship->vz = ctx.ship->jz;

    ctx.ship->cx = tx;
    ctx.ship->cy = ty;
    ctx.ship->cz = tz;

    ctx.ship->jx = tx;
    ctx.ship->jy = ty;
    ctx.ship->jz = tz;

    ctx.ship->ox = tx;
    ctx.ship->oy = ty;
    ctx.ship->oz = tz;

    space_learn_from_failure( ch, ctx.ship );
    	
}

    
void do_target(CHAR_DATA *ch, char *argument )
{
    scmd_data ctx;
    ctx.rest = argument;

    int chance;
    SHIP_DATA *target, *dship;
    std::string buf;
    bool isturret = FALSE;
    TURRET_DATA *turret;

    ctx.arg1 = std::string(ctx.rest);

    switch( ch->substate )
    {
    	  default:
            if (!space_require_ship_from_turret( ch , ctx )) return;
            if ( ctx.ship->gunseat != ch->in_room->vnum )
                isturret = TRUE;
            if (!space_require_ship_not_hyperspace( ch , ctx )) return;
            if (!space_require_ship_spaceship( ch , ctx )) return;    
            if (!space_requires_finish_launching( ch , ctx )) return;

            if ( autofly(ctx.ship) && ( !isturret || !check_pilot( ch, ctx.ship ) ) )
            {
                send_to_char("&RYou'll have to turn off the ships autopilot first....\n",ch);
                return;
            }

            if (ctx.arg1.empty())
            {
                send_to_char("&RYou need to specify a target!\n",ch);
                return;
            }

            if ( !str_cmp( ctx.arg1, "none") )
            {
                send_to_char("&GTarget set to none.\n",ch);
                if ( ch->in_room->vnum == ctx.ship->gunseat )
                    ctx.ship->target0 = NULL;
                if( ctx.ship->first_turret )
                    for ( turret = ctx.ship->first_turret; turret; turret = turret->next )
                        if ( ch->in_room->vnum == turret->roomvnum )
                            turret->target = NULL;

                return;
            }
            if (ctx.ship->shipclass > SHIP_PLATFORM)
                target = ship_in_room( ctx.ship->in_room , ctx.arg1 );
            else
                target = get_ship_here( ctx.ship->game, ctx.arg1, ctx.ship );


            if (  target == NULL )
            {
                send_to_char("&RThat ship isn't here!\n",ch);
                return;
            }

            if (  target == ctx.ship )
            {
                send_to_char("&RYou can't target your own ship!\n",ch);
                return;
            }

            if ( !str_cmp(ctx.ship->owner, "Trainer") && str_cmp(target->owner, "Trainer") )
            {
                send_to_char("&RTrainers can only target other trainers!!\n",ch);
                return;
            }
            if ( str_cmp(ctx.ship->owner, "Trainer") && !str_cmp(target->owner, "Trainer") )
            {
                send_to_char("&ROnly trainers can target other trainers!!\n",ch);
                return;
            }
            if( ctx.ship->shipclass <= SHIP_PLATFORM)
            {
              if ( !space_distance_ship_less_than( ctx.ship, target, 5000 ) )
              {
                  send_to_char("&RThat ship is too far away to target.\n",ch);
                  return;
              }
            }

            chance = IS_NPC(ch) ? ch->top_level
                : (int)  (ch->pcdata->learned[gsn_weaponsystems]) ;
            if ( number_percent( ) < chance )
    		    {
                send_to_char( "&GTracking target.\n", ch);
                act( AT_PLAIN, "$n makes some adjustments on the targeting computer.", ch,
                    NULL, ctx.arg1, TO_ROOM );
                add_timer ( ch , TIMER_DO_FUN , 1 , do_target , 1 );
                ch->dest_buf = str_dup(ctx.arg1);
                return;
            }
            send_to_char("&RYou fail to work the controls properly.\n",ch);
            learn_from_failure( ch, gsn_weaponsystems );
            return;

    	  case 1:
            if ( !ch->dest_buf )
              return;
            ctx.arg1 = std::string(( const char* ) ch->dest_buf);
            STR_DISPOSE( ch->dest_buf);
            break;

    	  case SUB_TIMER_DO_ABORT:
            STR_DISPOSE( ch->dest_buf );
            ch->substate = SUB_NONE;
            if ( !space_require_ship_from_cockpit( ch , ctx ) )
                  return;
            send_to_char("&RYour concentration is broken. You fail to lock onto your target.\n", ch);
            return;
    }

    ch->substate = SUB_NONE;

    if ( (ctx.ship = ship_from_turret(ch->game, ch->in_room->vnum)) == NULL )
        return;

    if (ctx.ship->shipclass > SHIP_PLATFORM)
        target = ship_in_room( ctx.ship->in_room , ctx.arg1 );
    else
        target = get_ship_here( ctx.ship->game, ctx.arg1, ctx.ship );

    if (  target == NULL || target == ctx.ship)
    {
        send_to_char("&RThe ship has left the starsystem. Targeting aborted.\n",ch);
        return;
    }

    if ( ch->in_room->vnum == ctx.ship->gunseat )
        ctx.ship->target0 = target;

    if( ctx.ship->first_turret )
        for( turret = ctx.ship->first_turret; turret; turret = turret->next )
            if ( ch->in_room->vnum == turret->roomvnum )
                turret->target = target;


    send_to_char( "&GTarget Locked.\n", ch);
    buf = str_printf("You are being targeted by %s." , ctx.ship->name);  
    echo_to_cockpit( AT_BLOOD , target , buf );
    echo_to_docked( AT_YELLOW , ctx.ship, "The ship's computer receives targeting data through the docking port link." );

    if ( ch->in_room->vnum == ctx.ship->gunseat )
      for( dship = first_ship; dship; dship = dship->next )
          if( dship->docked && dship->docked == ctx.ship )
              dship->target0 = target;
       	      
    sound_to_room( ch->in_room , "!!SOUND(targetlock)" );
    learn_from_success( ch, gsn_weaponsystems );
    	
    if ( autofly(target) && !target->target0)
    {
        buf = str_printf("You are being targeted by %s." , target->name);  
        echo_to_cockpit( AT_BLOOD , ctx.ship , buf );
        target->target0 = ctx.ship;
    }
}

int calc_manuever( SHIP_DATA *ship )
{
  int manuever;
  int weight;
  
  manuever = ship->mod->manuever;
  manuever *= 10;
  
  weight = (ship->weight/5);
  if( !weight )
    weight = 300*(ship->shipclass+1);
  
  weight += (ship->mod->maxhull/5)+(ship->energy/40);
  if( get_extmodule_count(ship) > 5 )
    weight += (ship->maxextmodules - get_extmodule_count(ship))*-20;
  
  if( weight <= 0 )
    weight = 300*(ship->shipclass+1);
  
  manuever *= 400;
  manuever /= (weight+1)*3;
  
  return manuever;
}

int calc_hit( SHIP_DATA *ship, SHIP_DATA *target )
{
  int distance = 0, chance = 0;
  int shipmanuever;
  int targetmanuever;
  int chancesize = 0;
  int chanceclass = 0;
  int chancespeed = 0;
  int chancemanuever = 0;
  int chancedistance = 0;
  int chancedivisor = 0;
  
  shipmanuever = calc_manuever( ship );
  targetmanuever = calc_manuever( target );
  
  distance = abs( (int) ( target->vx - ship->vx )) 
           + abs( (int) ( target->vy - ship->vy )) 
 	       + abs( (int) ( target->vz - ship->vz ));
  distance /= 3;

  chancesize -= (ship->weight+(ship->mod->maxhull/5));
  chancesize += (target->weight+(target->mod->maxhull/5));
  chancesize /= 200;
  
  chanceclass = ( target->shipclass - ship->shipclass ) *32;
  
  chancespeed = -1*abs((ship->currspeed - target->currspeed)/2);
  
  chancemanuever = ( shipmanuever-targetmanuever )/10;
  
  chancedistance -= distance/(10*(target->shipclass+1));
  
  chance = chancesize+chanceclass+chancespeed+chancemanuever+chancedistance;
  
  if ( chancesize )
    chancedivisor++;
  if ( chanceclass )
    chancedivisor++;
  if ( chancespeed )
    chancedivisor++;
  if ( chancemanuever )
    chancedivisor++;
  if ( chancedistance )
    chancedivisor++;
  
  chancedivisor--;
  
  chance /= chancedivisor;
  
  chance = URANGE( -49 , chance , 49 );

  return chance;
}

static bool fire_projectile_weapon( CHAR_DATA *ch,
                                    scmd_data &ctx,
                                    int chance,
                                    int origchance,
                                    ProjectileKind kind )
{
    SHIP_DATA *target;
    std::string buf;
    const char *no_ammo_msg;
    const char *reload_msg;
    const char *range_msg;
    const char *facing_msg;
    const char *incoming_msg_fmt;
    const char *observer_msg_fmt;
    const char *launch_msg;
    int max_delta;
    sh_int *ammo;
    int missile_type = -1;
    int capital_min_damage = 0;
    int capital_max_damage = 0;
    bool destroyed = FALSE;

    if ( ch->in_room->vnum != ctx.ship->gunseat )
        return false;

    switch ( kind )
    {
        default:
            return false;

        case ProjectileKind::MISSILE:
            if ( str_prefix( ctx.rest, "missile" ) )
                return false;
            no_ammo_msg = "&RYou have no missiles to fire!\n";
            reload_msg = "&RThe missiles are still reloading.\n";
            range_msg = "&RThat ship is out of missile range.\n";
            facing_msg = "&RMissiles can only fire in a forward. You'll need to turn your ship!\n";
            incoming_msg_fmt = "Incoming missile from %s.";
            observer_msg_fmt = "%s fires a missile towards %s.";
            launch_msg = "Missiles launched.";
            max_delta = 1000;
            ammo = &ctx.ship->missiles;
            missile_type = CONCUSSION_MISSILE;
            capital_min_damage = 75;
            capital_max_damage = 200;
            break;

        case ProjectileKind::TORPEDO:
            if ( str_prefix( ctx.rest, "torpedo" ) )
                return false;
            no_ammo_msg = "&RYou have no torpedos to fire!\n";
            reload_msg = "&RThe torpedos are still reloading.\n";
            range_msg = "&RThat ship is out of torpedo range.\n";
            facing_msg = "&RTorpedos can only fire in a forward direction. You'll need to turn your ship!\n";
            incoming_msg_fmt = "Incoming torpedo from %s.";
            observer_msg_fmt = "%s fires a torpedo towards %s.";
            launch_msg = "Missiles launched.";
            max_delta = 1000;
            ammo = &ctx.ship->torpedos;
            missile_type = PROTON_TORPEDO;
            capital_min_damage = 200;
            capital_max_damage = 300;
            break;

        case ProjectileKind::ROCKET:
            if ( str_prefix( ctx.rest, "rocket" ) )
                return false;
            no_ammo_msg = "&RYou have no rockets to fire!\n";
            reload_msg = "&RThe missiles are still reloading.\n";
            range_msg = "&RThat ship is out of rocket range.\n";
            facing_msg = "&RRockets can only fire forward. You'll need to turn your ship!\n";
            incoming_msg_fmt = "Incoming rocket from %s.";
            observer_msg_fmt = "%s fires a heavy rocket towards %s.";
            launch_msg = "Rocket launched.";
            max_delta = 800;
            ammo = &ctx.ship->rockets;
            missile_type = HEAVY_ROCKET;
            capital_min_damage = 450;
            capital_max_damage = 550;
            break;
    }

    if ( ctx.ship->missilestate == MISSILE_DAMAGED )
    {
        send_to_char( "&RThe ships missile launchers are damaged.\n", ch );
        return true;
    }

    if ( *ammo <= 0 )
    {
        send_to_char( no_ammo_msg, ch );
        return true;
    }

    if ( ctx.ship->missilestate != MISSILE_READY )
    {
        send_to_char( reload_msg, ch );
        return true;
    }

    target = space_get_target(ch, ctx.ship);
    if (!target) return true;

    target = ctx.ship->target0;

    if ( target_left_or_clear( ch, ctx.ship, target, &ctx.ship->target0 ) )
        return true;

    if ( target_out_of_coord_range( ch, ctx.ship, target, max_delta, range_msg ) )
        return true;

    if ( ctx.ship->shipclass < 2 && !is_facing( ctx.ship, target ) )
    {
        send_to_char( facing_msg, ch );
        return true;
    }

    chance = compute_projectile_hit_chance( chance, origchance, ctx.ship, target );

    act( AT_PLAIN, "$n presses the fire button.", ch, NULL, ctx.rest, TO_ROOM );

    if ( number_percent() > chance )
    {
        send_to_char( "&RYou fail to lock onto your target!", ch );
        ctx.ship->missilestate = MISSILE_RELOAD_2;
        return true;
    }

    --( *ammo );

    act( AT_PLAIN, "$n presses the fire button.", ch, NULL, ctx.rest, TO_ROOM );
    echo_to_cockpit( AT_YELLOW, ctx.ship, launch_msg );

    if ( ctx.ship->shipclass > SHIP_PLATFORM )
        echo_to_ship( AT_RED, target, "A large explosion vibrates through the ship." );

    echo_to_cockpit( AT_BLOOD, target, str_printf( incoming_msg_fmt, ctx.ship->name ) );

    buf = str_printf( observer_msg_fmt, ctx.ship->name, target->name );
    echo_ship_observers( AT_ORANGE, ctx.ship, target, buf );

    learn_from_success( ch, gsn_weaponsystems );

    if ( ctx.ship->shipclass == CAPITAL_SHIP || ctx.ship->shipclass == SHIP_PLATFORM )
        ctx.ship->missilestate = MISSILE_RELOAD;
    else
        ctx.ship->missilestate = MISSILE_FIRED;

    maybe_autofly_retarget( ctx.ship, target );

    if ( ctx.ship->shipclass <= SHIP_PLATFORM )
        new_missile( ctx.ship, target, ch, missile_type );
    else
        destroyed = damage_ship_ch( target, capital_min_damage, capital_max_damage, ch );

    if ( destroyed )
        ctx.ship->target0 = NULL;

    return true;
}

static bool fire_lasers( CHAR_DATA *ch, scmd_data &ctx, int basechance )
{
    SHIP_DATA *target;
    std::string buf;
    int chance = basechance;
    bool destroyed = FALSE;

    if ( ch->in_room->vnum != ctx.ship->gunseat || str_prefix( ctx.rest, "lasers" ) )
        return false;

    if ( ctx.ship->statet0 == LASER_DAMAGED )
    {
        send_to_char( "&RThe ships main laser is damaged.\n", ch );
        return true;
    }

    if ( ctx.ship->statet0 >= ctx.ship->mod->lasers )
    {
        send_to_char( "&RThe lasers are still recharging.\n", ch );
        return true;
    }

    target = space_get_target( ch, ctx.ship );
    if ( !target )
        return true;

    if ( target_left_or_clear( ch, ctx.ship, target, &ctx.ship->target0 ) )
        return true;

    if ( target_out_of_coord_range( ch, ctx.ship, target, 1000,
         "&RThat ship is out of laser range.\n" ) )
        return true;

    if ( ctx.ship->shipclass < 2 && !is_facing( ctx.ship, target ) )
    {
        send_to_char( "&RThe main laser can only fire forward. You'll need to turn your ship!\n", ch );
        return true;
    }

    ctx.ship->statet0++;

    chance /= 2;
    chance += calc_hit( ctx.ship, target );
    chance = URANGE( 1, chance, 99 );

    act( AT_PLAIN, "$n presses the fire button.", ch, NULL, ctx.rest, TO_ROOM );

    if ( number_percent() > chance )
    {
        echo_to_cockpit( AT_ORANGE, target,
            str_printf( "Lasers fire from %s at you but miss.", ctx.ship->name ) );
        echo_to_cockpit( AT_ORANGE, ctx.ship,
            str_printf( "The ships lasers fire at %s but miss.", target->name ) );
        space_learn_combat_from_failure( ch );

        buf = str_printf( "Laserfire from %s barely misses %s.",
                          ctx.ship->name, target->name );
        echo_ship_observers( AT_ORANGE, ctx.ship, target, buf );
        return true;
    }

    buf = str_printf( "Laserfire from %s hits %s.",
                      ctx.ship->name, target->name );
    echo_ship_observers( AT_ORANGE, ctx.ship, target, buf );

    echo_to_cockpit( AT_BLOOD, target,
        str_printf( "You are hit by lasers from %s!", ctx.ship->name ) );
    echo_to_cockpit( AT_YELLOW, ctx.ship,
        str_printf( "Your ships lasers hit %s!.", target->name ) );

    space_learn_combat_from_success( ch );
    if ( ctx.ship->shipclass > SHIP_PLATFORM )
        learn_from_success( ch, gsn_speedercombat );

    echo_to_ship( AT_RED, target, "A small explosion vibrates through the ship." );
    maybe_autofly_retarget( ctx.ship, target );

    if ( ctx.ship->shipclass == SHIP_PLATFORM )
        destroyed = damage_ship_ch( target, 100, 250, ch );
    else if ( ctx.ship->shipclass == CAPITAL_SHIP && target->shipclass < CAPITAL_SHIP )
        destroyed = damage_ship_ch( target, 50, 200, ch );
    else
        destroyed = damage_ship_ch( target, 10, 50, ch );

    if ( destroyed )
        ctx.ship->target0 = NULL;

    return true;
}

static bool fire_ions( CHAR_DATA *ch, scmd_data &ctx, int basechance )
{
    SHIP_DATA *target;
    std::string buf;
    int chance = basechance;
    bool destroyed = FALSE;

    if ( ch->in_room->vnum != ctx.ship->gunseat || str_prefix( ctx.rest, "ions" ) )
        return false;

    if ( ctx.ship->statet0 == LASER_DAMAGED )
    {
        send_to_char( "&RThe ships main weapons are damaged.\n", ch );
        return true;
    }

    if ( ctx.ship->mod->ions <= 0 )
    {
        send_to_char( "&RYou have no ion cannons to fire.\n", ch );
        return true;
    }

    if ( ctx.ship->statei0 >= ctx.ship->mod->ions )
    {
        send_to_char( "&RThe ion cannons are still recharging.\n", ch );
        return true;
    }

    target = space_get_target( ch, ctx.ship );
    if ( !target )
        return true;

    if ( target_left_or_clear( ch, ctx.ship, target, &ctx.ship->target0 ) )
        return true;

    if ( target_out_of_coord_range( ch, ctx.ship, target, 1000,
         "&RThat ship is out of ion range.\n" ) )
        return true;

    if ( ctx.ship->shipclass < 2 && !is_facing( ctx.ship, target ) )
    {
        send_to_char( "&RThe main ion cannon can only fire forward. You'll need to turn your ship!\n", ch );
        return true;
    }

    ctx.ship->statei0++;

    chance /= 2;
    chance += calc_hit( ctx.ship, target );
    chance = URANGE( 1, chance, 99 );

    act( AT_PLAIN, "$n presses the fire button.", ch, NULL, ctx.rest, TO_ROOM );

    if ( number_percent() > chance )
    {
        echo_to_cockpit( AT_ORANGE, target,
            str_printf( "Ion cannons fire from %s at you, but the blue plasma narrowly misses.",
                        ctx.ship->name ) );
        echo_to_cockpit( AT_ORANGE, ctx.ship,
            str_printf( "The ships ion cannons fire at %s but the blue plasma narrowly misses.",
                        target->name ) );

        space_learn_combat_from_failure( ch );

        buf = str_printf( "Blue ion plasma from %s narrowly misses %s.",
                          ctx.ship->name, target->name );
        echo_ship_observers( AT_ORANGE, ctx.ship, target, buf );
        return true;
    }

    buf = str_printf( "Blue plasma from %s engulfs %s.",
                      ctx.ship->name, target->name );
    echo_ship_observers( AT_ORANGE, ctx.ship, target, buf );

    echo_to_cockpit( AT_BLOOD, target,
        str_printf( "You are engulfed by ion energy from %s!", ctx.ship->name ) );
    echo_to_cockpit( AT_YELLOW, ctx.ship,
        str_printf( "Blue plasma from your ship engulf %s!.", target->name ) );
    space_learn_combat_from_success( ch );

    if ( ctx.ship->shipclass > SHIP_PLATFORM )
        learn_from_success( ch, gsn_speedercombat );

    echo_to_ship( AT_RED, target, "A small explosion vibrates through the ship." );
    maybe_autofly_retarget( ctx.ship, target );

    if ( ctx.ship->shipclass == SHIP_PLATFORM )
        destroyed = damage_ship_ch( target, -200, -50, ch );
    else if ( ctx.ship->shipclass == CAPITAL_SHIP && target->shipclass <= MIDSIZE_SHIP )
        destroyed = damage_ship_ch( target, -200, -50, ch );
    else
        destroyed = damage_ship_ch( target, -75, -10, ch );

    if ( destroyed )
        ctx.ship->target0 = NULL;

    return true;
}

static bool fire_turret_lasers( CHAR_DATA *ch, scmd_data &ctx, int basechance, int origchance )
{
    SHIP_DATA   *target;
    TURRET_DATA *turret;
    std::string  buf;

    if ( !ctx.ship->first_turret )
        return false;

    for ( turret = ctx.ship->first_turret; turret; turret = turret->next )
    {
        int distance;
        int turretchance;

        if ( ch->in_room->vnum != turret->roomvnum || str_prefix( ctx.rest, "lasers" ) )
            continue;

        if ( turret->state == LASER_DAMAGED )
        {
            send_to_char( "&RThe ships turret is damaged.\n", ch );
            return true;
        }

        if ( turret->state > ctx.ship->shipclass )
        {
            send_to_char( "&RThe turret is recharging.\n", ch );
            return true;
        }

        target = space_get_turret_target( ch, turret );
        if ( !target )
            return true;

        if ( target_left_or_clear( ch, ctx.ship, target, &turret->target ) )
            return true;

        if ( target_out_of_coord_range( ch, ctx.ship, target, 1000,
             "&RThat ship is out of range.\n" ) )
            return true;

        turret->state++;

        distance = ship_target_distance( ctx.ship, target );

        turretchance = basechance;
        turretchance += (target->shipclass - CAPITAL_SHIP) + 1;
        turretchance += ctx.ship->currspeed - target->currspeed;
        turretchance += ( 100 + ( turret->type * 50 ) ) - target->mod->manuever;
        turretchance -= distance / ( 10 * ( target->shipclass + 1 ) );
        turretchance -= origchance;
        turretchance /= 2;
        turretchance += origchance;
        turretchance = URANGE( 1, turretchance, 99 );

        act( AT_PLAIN, "$n presses the fire button.", ch, NULL, ctx.rest, TO_ROOM );

        if ( number_percent() > turretchance )
        {
            buf = str_printf( turretstats[turret->type].targetmissmsg, ctx.ship->name );
            echo_to_cockpit( AT_ORANGE, target, buf );

            buf = str_printf( turretstats[turret->type].selfmissmsg, target->name );
            echo_to_cockpit( AT_ORANGE, ctx.ship, buf );

            buf = str_printf( turretstats[turret->type].observermissmsg,
                              ctx.ship->name, target->name );
            echo_ship_observers( AT_ORANGE, ctx.ship, target, buf );
            space_learn_combat_from_failure( ch );
            return true;
        }

        echo_to_cockpit( AT_BLOOD, target,
            str_printf( turretstats[turret->type].targetdammsg, ctx.ship->name ) );
        echo_to_cockpit( AT_YELLOW, ctx.ship,
            str_printf( turretstats[turret->type].selfdammsg, target->name ) );

        buf = str_printf( turretstats[turret->type].observerdammsg,
                          ctx.ship->name, target->name );
        echo_to_cockpit( AT_ORANGE, ctx.ship, buf );
        echo_ship_observers( AT_ORANGE, ctx.ship, target, buf );

        space_learn_combat_from_success( ch );

        echo_to_ship( AT_RED, target, "A small explosion vibrates through the ship." );

        damage_ship_ch( target,
                        turretstats[turret->type].mindamage,
                        turretstats[turret->type].maxdamage,
                        ch );

        maybe_autofly_retarget( ctx.ship, target );
        return true;
    }

    return false;
}

void do_fire(CHAR_DATA *ch, char *argument )
{
    scmd_data ctx;
    ctx.rest = argument;  
    int basechance, origchance;
    std::string buf;
    bool isturret = FALSE;    
    
    if ( !space_require_ship_from_turret( ch , ctx )) return;

		if ( ctx.ship->gunseat != ch->in_room->vnum )
		  isturret = TRUE;
        
    if (!space_require_ship_not_hyperspace( ch , ctx )) return;
    if (!space_requires_finish_launching( ch , ctx )) return;
    int required_energy = 5;
    if (!space_require_ship_energy( ch , ctx , required_energy )) return;  
  
    if ( autofly(ctx.ship) && !isturret )
    {
        send_to_char("&RYou'll have to turn off the ships autopilot first.\n",ch);
        return;
    }
    	              
    basechance = space_chance_combat( ch, ctx.ship );
    origchance = basechance;

    if ( ctx.ship->shipclass > SHIP_PLATFORM && !IS_NPC( ch ) )
    {
        if ( ch->pcdata->learned[gsn_speeders] == 100 )
            basechance -= 100 - ch->pcdata->learned[gsn_speedercombat];
        else
            basechance = 0;
    }
    
    if (fire_lasers( ch, ctx, basechance ) )
        return;

    if ( fire_ions( ch, ctx, basechance ) )
        return;

    if ( fire_projectile_weapon( ch, ctx, basechance, origchance, ProjectileKind::MISSILE ) ) return;
    if ( fire_projectile_weapon( ch, ctx, basechance, origchance, ProjectileKind::TORPEDO ) ) return;
    if ( fire_projectile_weapon( ch, ctx, basechance, origchance, ProjectileKind::ROCKET ) ) return;
        
    if ( fire_turret_lasers( ch, ctx, basechance, origchance ) ) return;

        send_to_char( "&RYou can't fire that!\n" , ch);
    }


void do_calculate(CHAR_DATA *ch, char *argument )
{


    std::string buf;  
    int chance , distance = 0;
    SPACE_DATA *spaceobj, *spaceobject;
    bool found = FALSE;

    scmd_data ctx;
    ctx.rest = argument;    
    ctx.rest = one_argument( ctx.rest , ctx.arg1);
    ctx.rest = one_argument( ctx.rest , ctx.arg2);
    ctx.rest = one_argument( ctx.rest , ctx.arg3);
    
    if (!space_require_ship_from_cockpit( ch , ctx )) return;
    if (!space_require_ship_from_navseat( ch , ctx )) return;
    if (!space_require_ship_autopilot_off( ch , ctx )) return;
    if (!space_require_ship_spaceship( ch , ctx )) return;    
    if (!space_require_ship_not_landed( ch , ctx )) return;
    if (!space_require_ship_not_hyperspace( ch , ctx )) return;
    if (!space_requires_finish_launching( ch , ctx )) return;    

    if (ctx.ship->mod->hyperspeed == 0)
    {
        send_to_char("&RThis ship is not equipped with a hyperdrive!\n",ch);
        return;   
    }

    if (ctx.arg1.empty() )
    {
        send_to_char("&WFormat: Calculate <spaceobject> <entry x> <entry y> <entry z>\n",ch);
        return;
    }
    chance = IS_NPC(ch) ? ch->top_level
        : (int)  (ch->pcdata->learned[gsn_navigation]) ;
    if ( number_percent( ) > chance )
    {
        send_to_char("&RYou cant seem to figure the charts out today.\n",ch);
        learn_from_failure( ch, gsn_navigation );
        return;
    }

    if( !is_number(ctx.arg1) && ctx.arg1[0] != '-')
    {
        ctx.ship->currjump = spaceobject_from_name( ch->game, ctx.arg1 );
        if ( !ctx.arg2.empty() && is_number(ctx.arg2) )
            distance = strtoi(ctx.arg2);
        if( ctx.ship->currjump )
        {
            ctx.ship->jx = ctx.ship->currjump->xpos;
            ctx.ship->jy = ctx.ship->currjump->ypos;
            ctx.ship->jz = ctx.ship->currjump->zpos;
            found = TRUE;
        }
    }  
    else if( !ctx.arg2.empty() && !ctx.arg3.empty() )
    {
        ctx.ship->jx = strtoi(ctx.arg1);
        ctx.ship->jy = strtoi(ctx.arg2);
        ctx.ship->jz = strtoi(ctx.arg3);
        found = TRUE;
    }
    else
    {
        send_to_char("&WFormat: Calculate <spaceobject> \n        Calculate <entry x> <entry y> <entry z>&R&w\n",ch);
        return;
    }    

    spaceobject = ctx.ship->currjump;
        
    if ( !found )
    {
        send_to_char( "&RYou can't seem to find that spatial object on your charts.\n", ch);
        ctx.ship->currjump = NULL;
        return;
    }
    if (spaceobject && spaceobject->trainer && (ctx.ship->shipclass != SHIP_TRAINER))
    {
        send_to_char( "&RYou can't seem to find that spatial object on your charts.\n", ch);
        ctx.ship->currjump = NULL;
        return;
    }
    if (ctx.ship->shipclass == SHIP_TRAINER && spaceobject && !spaceobject->trainer )
    {
        send_to_char( "&RYou can't seem to find that starsystem on your charts.\n", ch);
        ctx.ship->currjump = NULL;
        return;
    }
    int accuracy = number_range ( ((ctx.ship->mod->astro_array) - 300) , (300-(ctx.ship->mod->astro_array)) );
    distance = (distance ? distance : (spaceobject && spaceobject->gravity ? spaceobject->gravity*5 : 0 ) );
    ctx.ship->jx += accuracy;
    ctx.ship->jy += accuracy;
    ctx.ship->jz += accuracy;
    ctx.ship->jx += distance;
    ctx.ship->jy += distance;
    ctx.ship->jz += distance;

    for ( spaceobj = first_spaceobject; spaceobj; spaceobj = spaceobj->next )
      if ( !spaceobj->trainer && distance && strcmp(spaceobj->name,"") && 
                abs( (int) ( ctx.ship->jx - spaceobj->xpos )) < (spaceobj->gravity*4) && 
                abs( (int) ( ctx.ship->jy - spaceobj->ypos )) < (spaceobj->gravity*4) &&
                abs( (int) ( ctx.ship->jz - spaceobj->zpos )) < (spaceobj->gravity*4) )
          {
              echo_to_cockpit( AT_RED, ctx.ship, "WARNING.. Jump coordinates too close to stellar object.");
              echo_to_cockpit( AT_RED, ctx.ship, "WARNING.. Hyperjump NOT set.");
              ctx.ship->currjump = NULL;
              return;
          }              

    for( spaceobject = first_spaceobject; spaceobject; spaceobject = spaceobject->next )
      if( space_in_range( ctx.ship, spaceobject ) )
      {
        ctx.ship->currjump = spaceobject;
        break;
      }
    if( !spaceobject )
      ctx.ship->currjump = ctx.ship->spaceobject;

    if( ctx.ship->jx > MAX_COORD_S || ctx.ship->jy > MAX_COORD_S || ctx.ship->jz > MAX_COORD_S ||
    ctx.ship->jx < -MAX_COORD_S || ctx.ship->jy < -MAX_COORD_S || ctx.ship->jz < -MAX_COORD_S )
    {
                    echo_to_cockpit( AT_RED, ctx.ship, "WARNING.. Jump coordinates outside of the known galaxy.");
                    echo_to_cockpit( AT_RED, ctx.ship, "WARNING.. Hyperjump NOT set.");
                    ctx.ship->currjump = NULL;
                    return;
    }
    
    ctx.ship->hyperdistance  = abs( (int) ( ctx.ship->vx - ctx.ship->jx )) ;
    ctx.ship->hyperdistance += abs( (int) ( ctx.ship->vy - ctx.ship->jy )) ;
    ctx.ship->hyperdistance += abs( (int) ( ctx.ship->vz - ctx.ship->jz )) ;
    ctx.ship->hyperdistance /= 200;

    if (ctx.ship->hyperdistance<100)
      ctx.ship->hyperdistance = 100;
      
    ctx.ship->orighyperdistance = ctx.ship->hyperdistance;

    sound_to_room( ch->in_room , "!!SOUND(computer)" );

    buf = str_printf( "&GHyperspace course set. Estimated distance: %d\nReady for the jump to lightspeed.\n", ctx.ship->hyperdistance );
    send_to_char( buf, ch);
    echo_to_docked( AT_YELLOW , ctx.ship, "The docking port link shows a new course being calculated." );

    act( AT_PLAIN, "$n does some calculations using the ships computer.", ch,
		        NULL, ctx.rest , TO_ROOM );
	                
    learn_from_success( ch, gsn_navigation );

    WAIT_STATE( ch , 2*PULSE_VIOLENCE );	
}

void do_calculate_diff(CHAR_DATA *ch, char *argument )
{
    scmd_data ctx;
    ctx.rest = argument;    
    ctx.rest = one_argument( ctx.rest , ctx.arg1);
    ctx.rest = one_argument( ctx.rest , ctx.arg2);
    ctx.rest = one_argument( ctx.rest , ctx.arg3);
    
    if (!space_require_ship_from_cockpit( ch , ctx )) return;
    if (!space_require_ship_from_navseat( ch , ctx )) return;
    if (!space_require_ship_autopilot_off( ch , ctx )) return;
    if (!space_require_ship_spaceship( ch , ctx )) return;    
    if (!space_require_ship_not_landed( ch , ctx )) return;
    if (!space_require_ship_not_hyperspace( ch , ctx )) return;
    if (!space_requires_finish_launching( ch , ctx )) return;    

    std::string buf;  
    int chance , distance = 0;
    SPACE_DATA *spaceobj, *spaceobject;
    bool found = FALSE;
      
    if (ctx.ship->mod->hyperspeed == 0)
    {
        send_to_char("&RThis ship is not equipped with a hyperdrive!\n",ch);
        return;   
    }
    if (ctx.ship->spaceobject == NULL)
    {
        send_to_char("&RYou can only do that in realspace.\n",ch);
        return;
    }
    if (ctx.arg1.empty())
    {
        send_to_char("&WFormat: Calculate <spaceobject> <entry x> <entry y> <entry z>\n&wPossible destinations:\n",ch);
/*    	            for ( spaceobj = first_spaceobject; spaceobj; spaceobj = spaceobj->next )
          {
              set_char_color( AT_NOTE, ch );
              ch_printf(ch,"%-30s %d\n",spaceobj->name,
                        (abs(spaceobj->xpos - ship->spaceobj->xpos)+
                        abs(spaceobj->ypos - ship->spaceobj->ypos))/2);
              count++;
          }
          if ( !count )
          {
              send_to_char( "No spacial objects found.\n", ch );
          }
*/                  return;
    }
    chance = IS_NPC(ch) ? ch->top_level
        : (int)  (ch->pcdata->learned[gsn_navigation]) ;
    if ( number_percent( ) > chance )
    {
        send_to_char("&RYou cant seem to figure the charts out today.\n",ch);
        learn_from_failure( ch, gsn_navigation );
        return;
    }

    if( !ctx.arg2.empty() && !ctx.arg3.empty())
    {
        ctx.ship->jx = ctx.ship->vx + strtoi(ctx.arg1);
        ctx.ship->jy = ctx.ship->vy + strtoi(ctx.arg2);
        ctx.ship->jz = ctx.ship->vz + strtoi(ctx.arg3);
        found = TRUE;
    }
    else
    {
        send_to_char("&WFormat: Calculate x y z&R&w\n",ch);
        return;
    }    

    spaceobject = ctx.ship->currjump;
        
    if ( !found )
    {
      send_to_char( "&RYou can't seem to find that spatial object on your charts.\n", ch);
      ctx.ship->currjump = NULL;
      return;
    }

    int accuracy = number_range ( ((ctx.ship->mod->astro_array) - 300) , (300-(ctx.ship->mod->astro_array)) );
    distance = (distance ? distance : (spaceobject && spaceobject->gravity ? spaceobject->gravity*5 : 0 ) );
    ctx.ship->jx += accuracy;
    ctx.ship->jy += accuracy;
    ctx.ship->jz += accuracy;
    ctx.ship->jx += distance;
    ctx.ship->jy += distance;
    ctx.ship->jz += distance;

 	for ( spaceobj = first_spaceobject; spaceobj; spaceobj = spaceobj->next )
      if ( !spaceobj->trainer && distance && strcmp(spaceobj->name,"") && 
            abs( (int) ( ctx.ship->jx - spaceobj->xpos )) < (spaceobj->gravity*4) && 
            abs( (int) ( ctx.ship->jy - spaceobj->ypos )) < (spaceobj->gravity*4) &&
            abs( (int) ( ctx.ship->jz - spaceobj->zpos )) < (spaceobj->gravity*4) )
      {
          echo_to_cockpit( AT_RED, ctx.ship, "WARNING.. Jump coordinates too close to stellar object.");
          echo_to_cockpit( AT_RED, ctx.ship, "WARNING.. Hyperjump NOT set.");
          ctx.ship->currjump = NULL;
          return;
      }              

    for( spaceobject = first_spaceobject; spaceobject; spaceobject = spaceobject->next )
      if( space_in_range( ctx.ship, spaceobject ) )
      {
          ctx.ship->currjump = spaceobject;
          break;
      }
    if( !spaceobject )
        ctx.ship->currjump = ctx.ship->spaceobject;

    if( ctx.ship->jx > MAX_COORD_S || ctx.ship->jy > MAX_COORD_S || ctx.ship->jz > MAX_COORD_S ||
        ctx.ship->jx < -MAX_COORD_S || ctx.ship->jy < -MAX_COORD_S || ctx.ship->jz < -MAX_COORD_S )
    {
        echo_to_cockpit( AT_RED, ctx.ship, "WARNING.. Jump coordinates outside of the known galaxy.");
        echo_to_cockpit( AT_RED, ctx.ship, "WARNING.. Hyperjump NOT set.");
        ctx.ship->currjump = NULL;
        return;
    }
    


    ctx.ship->hyperdistance  = abs( (int ) ( ctx.ship->vx - ctx.ship->jx )) ;
    ctx.ship->hyperdistance += abs( (int ) ( ctx.ship->vy - ctx.ship->jy )) ;
    ctx.ship->hyperdistance += abs( (int ) ( ctx.ship->vz - ctx.ship->jz )) ;
    ctx.ship->hyperdistance /= 200;

    if (ctx.ship->hyperdistance<100)
        ctx.ship->hyperdistance = 100;
      
    ctx.ship->orighyperdistance = ctx.ship->hyperdistance;

    sound_to_room( ch->in_room , "!!SOUND(computer)" );

    buf = str_printf( "&GHyperspace course set. Estimated distance: %d\nReady for the jump to lightspeed.\n", ctx.ship->hyperdistance );
    send_to_char( buf, ch);
    echo_to_docked( AT_YELLOW , ctx.ship, "The docking port link shows a new course being calculated." );

    act( AT_PLAIN, "$n does some calculations using the ships computer.", ch,
		        NULL, ctx.rest , TO_ROOM );
	                
    learn_from_success( ch, gsn_navigation );

    WAIT_STATE( ch , 2*PULSE_VIOLENCE );	
}


void do_recharge(CHAR_DATA *ch, char *argument )
{
    scmd_data ctx;
    ctx.rest = argument;    
    ctx.rest = one_argument( ctx.rest , ctx.arg1);
    ctx.rest = one_argument( ctx.rest , ctx.arg2);
    ctx.rest = one_argument( ctx.rest , ctx.arg3);
    
    if (!space_require_ship_from_navseat( ch , ctx )) return;
    if (!space_require_ship_autopilot_off( ch , ctx )) return;
    if (!space_require_ship_spaceship( ch , ctx )) return;    
    if (!space_require_ship_not_landed( ch , ctx )) return;
    if (!space_require_ship_not_hyperspace( ch , ctx )) return;
    if (!space_requires_finish_launching( ch , ctx )) return;     
    int recharge;
    int chance;

    
    if (!space_require_ship_from_cockpit( ch , ctx )) return;
    if (!space_require_ship_from_coseat( ch , ctx )) return;
    if (!space_require_ship_not_disabled( ch , ctx )) return;    
    if (!space_require_ship_energy( ch , ctx , 100 )) return;          
          
    chance = IS_NPC(ch) ? ch->top_level
              : (int) (ch->pcdata->learned[gsn_shipsystems]);
    if ( number_percent( ) > chance )
    {
        send_to_char("&RYou fail to work the controls properly.\n",ch);
        learn_from_failure( ch, gsn_shipsystems );
        return;	
    }
        
    send_to_char( "&GRecharging shields..\n", ch);
    act( AT_PLAIN, "$n pulls back a lever on the control panel.", ch,
         NULL, ctx.rest , TO_ROOM );
    
    learn_from_success( ch, gsn_shipsystems );
     
    recharge  = 25+ctx.ship->shipclass*25;
    recharge  = UMIN(  ctx.ship->mod->maxshield-ctx.ship->shield , recharge );
    ctx.ship->shield += recharge;
    ctx.ship->energy -= ( recharge*2 + recharge * ctx.ship->shipclass );        
}


void do_repairship(CHAR_DATA *ch, char *argument )
{
    scmd_data ctx;
    ctx.rest = argument;  

    int chance, change;
    TURRET_DATA *turret;
    ctx.arg1 = ctx.rest;

    switch( ch->substate )
    {
    	default:
          if ( !space_require_ship_from_engineroom( ch , ctx )) return;

          if ( str_cmp( ctx.arg1 , "hull" ) && str_cmp( ctx.arg1 , "drive" ) &&
                str_cmp( ctx.arg1 , "launcher" ) && str_cmp( ctx.arg1 , "laser" ) &&
                str_cmp( ctx.arg1 , "turret" ) &&
                str_cmp( ctx.arg1 , "docking" ) && str_cmp( ctx.arg1 , "tractor" ) )
          {
              send_to_char("&RYou need to spceify something to repair:\n",ch);
              send_to_char("&rTry: hull, drive, launcher, laser, docking, tractor, or turret\n",ch);
              return;
          }

          chance = IS_NPC(ch) ? ch->top_level
              : (int) (ch->pcdata->learned[gsn_shipmaintenance]);
          if ( number_percent( ) < chance )
    		  {
              send_to_char( "&GYou begin your repairs\n", ch);
              act( AT_PLAIN, "$n begins repairing the ships $T.", ch,
                NULL, ctx.arg1 , TO_ROOM );
              if ( !str_cmp(ctx.arg1,"hull") )
                add_timer ( ch , TIMER_DO_FUN , 15 , do_repairship , 1 );
              else
                add_timer ( ch , TIMER_DO_FUN , 5 , do_repairship , 1 );
              ch->dest_buf = str_dup(ctx.arg1);
              return;
	        }
	        send_to_char("&RYou fail to locate the source of the problem.\n",ch);
	        learn_from_failure( ch, gsn_shipmaintenance );
    	   	return;

    	case 1:
          if ( !ch->dest_buf )
              return;
          ctx.arg1 = (const char * ) ch->dest_buf;
          STR_DISPOSE( ch->dest_buf);
          break;

    	case SUB_TIMER_DO_ABORT:
          STR_DISPOSE( ch->dest_buf );
          ch->substate = SUB_NONE;
          if ( (ctx.ship = ship_from_cockpit(ch->game, ch->in_room->vnum)) == NULL )
                return;
          send_to_char("&RYou are distracted and fail to finish your repairs.\n", ch);
          return;
    }

    ch->substate = SUB_NONE;
    
    if ( !space_require_ship_from_engineroom( ch , ctx )) return;
    
    if ( !str_cmp(ctx.arg1,"hull") )
    {
        change = URANGE( 0 , 
                    number_range( (IS_NPC(ch) ? ch->top_level
	                 : (int) (ch->pcdata->learned[gsn_shipmaintenance]) / 2 ) , 
	                 (IS_NPC(ch) ? ch->top_level : (int) (ch->pcdata->learned[gsn_shipmaintenance]) ) ),
                    ( ctx.ship->mod->maxhull - ctx.ship->hull ) );
        ctx.ship->hull += change;
        ch_printf( ch, "&GRepair complete.. Hull strength inreased by %d points.\n", change );
    }
    
    if ( !str_cmp(ctx.arg1,"drive") )
    {  
        if (ctx.ship->location == ctx.ship->lastdoc)
            space_set_shipstate(ctx.ship, SHIP_LANDED);
        else if ( ctx.ship->shipstate == SHIP_HYPERSPACE )
            send_to_char("You realize after working that it would be a bad idea to do this while in hyperspace.\n", ch);		
        else     
            space_set_shipstate(ctx.ship, SHIP_READY);
        send_to_char("&GShips drive repaired.\n", ch);		
    }

    if ( !str_cmp(ctx.arg1,"docking") )
    {  
        ctx.ship->statetdocking = SHIP_READY;
        send_to_char("&GDocking bay repaired.\n", ch);
    }
    if ( !str_cmp(ctx.arg1,"tractor") )
    {  
        ctx.ship->statettractor = SHIP_READY;
        send_to_char("&GTractorbeam repaired.\n", ch);
    }
    if ( !str_cmp(ctx.arg1,"launcher") )
    {  
        ctx.ship->missilestate = MISSILE_READY;
        send_to_char("&GMissile launcher repaired.\n", ch);
    }
    
    if ( !str_cmp(ctx.arg1,"laser") )
    {  
        ctx.ship->statet0 = LASER_READY;
        send_to_char("&GMain laser repaired.\n", ch);
    }

    if ( !str_cmp(ctx.arg1,"turret") )
    {  
      if( ctx.ship->first_turret )
        for( turret = ctx.ship->first_turret; turret; turret = turret->next )
          if( ch->in_room->vnum == turret->roomvnum )
          {
              turret->state = LASER_READY;
              send_to_char("&GLaser Turret repaired.\n", ch);
          }
    }
    
    act( AT_PLAIN, "$n finishes the repairs.", ch,
         NULL, ctx.arg1 , TO_ROOM );

    learn_from_success( ch, gsn_shipmaintenance );
    	
}

void do_addpilot(CHAR_DATA *ch, char *argument )
{
    scmd_data ctx;
    ctx.rest = argument;

    if (!space_require_ship_from_cockpit( ch , ctx )) return;  
    int chance;
     
    chance = number_percent( );
    
    if ( IS_NPC(ch) || chance >= ch->pcdata->learned[gsn_slicing] )
    { 
        if (!space_require_ship_ownership( ch , ctx )) return;
            return;
    }

    if (ctx.rest.empty())
    {
        send_to_char( "&RAdd which pilot?\n" ,ch );
        return;
    }
    
    if ( chance < ch->pcdata->learned[gsn_slicing] ) 
        learn_from_success( ch, gsn_slicing );

    if ( str_cmp( ctx.ship->pilot , "" ) )
    {
        if ( str_cmp( ctx.ship->copilot , "" ) )
        {
            send_to_char( "&RYou already have a pilot and copilot..\n" ,ch );
            send_to_char( "&RTry rempilot first.\n" ,ch );
            return;
        }
        
        STRFREE( ctx.ship->copilot );
        ctx.ship->copilot = STRALLOC( ctx.rest );
        send_to_char( "Copilot Added.\n", ch );
        save_ship( ctx.ship );
        return;
        
        return;
   }
   
   STRFREE( ctx.ship->pilot );
   ctx.ship->pilot = STRALLOC( ctx.rest );
   send_to_char( "Pilot Added.\n", ch );
   save_ship( ctx.ship );

}

void do_rempilot(CHAR_DATA *ch, char *argument )
{
    scmd_data ctx;
    ctx.rest = argument;

    if (!space_require_ship_from_cockpit( ch , ctx )) return;    

    int chance;

    if  ( ctx.ship->shipclass == SHIP_PLATFORM )
    {
        send_to_char( "&RYou can't do that here.\n" , ch );
        return;
    }   
    chance = number_percent( );
    if ( IS_NPC(ch) || chance >= ch->pcdata->learned[gsn_slicing] )
    { 
        if (!space_require_ship_ownership( ch , ctx )) return;
            return;
    }

    if (ctx.rest.empty())
    {
          send_to_char( "&RRemove which pilot?\n" ,ch );
          return;
    }

    if ( chance < ch->pcdata->learned[gsn_slicing] ) 
        learn_from_success( ch, gsn_slicing );
     
    if ( !str_cmp( ctx.ship->pilot , ctx.rest ) )
    {
        STRFREE( ctx.ship->pilot );
        ctx.ship->pilot = STRALLOC( "" );
        send_to_char( "Pilot Removed.\n", ch );
        save_ship( ctx.ship );
        return;
    }       
   
    if ( !str_cmp( ctx.ship->copilot , ctx.rest ) )
    {      
        STRFREE( ctx.ship->copilot );
        ctx.ship->copilot = STRALLOC( "" );
        send_to_char( "Copilot Removed.\n", ch );
        save_ship( ctx.ship );
        return;
    }    

    send_to_char( "&RThat person isn't listed as one of the ships pilots.\n" ,ch );

}

static void append_spaceobj_line( std::string &out, const SHIP_DATA *ship, const SPACE_DATA *spaceobj )
{
    out += str_printf(
        "%-40s%.0f %.0f %.0f\n"
        "%-40s%6.0f %6.0f %6.0f\n",
        spaceobj->name,
        spaceobj->xpos,
        spaceobj->ypos,
        spaceobj->zpos,
        "",
        ( spaceobj->xpos - ship->vx ),
        ( spaceobj->ypos - ship->vy ),
        ( spaceobj->zpos - ship->vz )
    );
}

void show_space_objects( CHAR_DATA *ch, SHIP_DATA *ship )
{
    SPACE_DATA *spaceobj;
    std::string suns;
    std::string planets;
    std::string others;

    for ( spaceobj = first_spaceobject; spaceobj; spaceobj = spaceobj->next )
    {
        if ( !space_in_range( ship, spaceobj ) || !str_cmp( spaceobj->name, "" ) )
            continue;

        if ( spaceobj->type == SPACE_SUN )
            append_spaceobj_line( suns, ship, spaceobj );
        else if ( spaceobj->type == SPACE_PLANET )
            append_spaceobj_line( planets, ship, spaceobj );
        else if ( spaceobj->type > SPACE_PLANET )
            append_spaceobj_line( others, ship, spaceobj );
    }

    set_char_color( AT_RED, ch );
    send_to_char( suns, ch );

    set_char_color( AT_LBLUE, ch );
    send_to_char( planets, ch );

    ch_printf( ch, "\n" );

    set_char_color( AT_WHITE, ch );
    send_to_char( others, ch );

    ch_printf( ch, "\n" );
}

void do_radar( CHAR_DATA *ch, char *argument )
{
    scmd_data ctx;
    ctx.rest = argument;

    if (!space_require_ship_from_cockpit( ch , ctx )) return;  

    SHIP_DATA *target;
    int chance;
    MISSILE_DATA *missile;    
    
    if (!space_require_ship_spaceship( ch , ctx )) return;
    if (!space_require_ship_not_landed( ch , ctx )) return;
    if (!space_require_ship_not_hyperspace( ch , ctx )) return;
    if (!space_requires_in_system( ch , ctx )) return;
          
    chance = IS_NPC(ch) ? ch->top_level
          : (int)  (ch->pcdata->learned[gsn_navigation]) ;
    if ( number_percent( ) > chance )
    {
        send_to_char("&RYou fail to work the controls properly.\n",ch);
        learn_from_failure( ch, gsn_navigation );
        return;	
    }
    
    act( AT_PLAIN, "$n checks the radar.", ch,
         NULL, ctx.rest , TO_ROOM );
     
    show_space_objects( ch, ctx.ship );

    for ( target = first_ship; target; target = target->next )
    {
        std::string outbuf;
        if ( target != ctx.ship && target->spaceobject )
        {
            if ( abs( (int) ( target->vx - ctx.ship->vx )) < 100*(ctx.ship->mod->sensor+10)*((target->shipclass == SHIP_DEBRIS ? 2 : target->shipclass)+1) &&
            abs( (int) ( target->vy - ctx.ship->vy )) < 100*(ctx.ship->mod->sensor+10)*((target->shipclass == SHIP_DEBRIS ? 2 : target->shipclass)+1) &&
            abs( (int) ( target->vz - ctx.ship->vz )) < 100*(ctx.ship->mod->sensor+10)*((target->shipclass == SHIP_DEBRIS ? 2 : target->shipclass)+1) )
            {
              outbuf = str_printf("%s",target->name);
              ch_printf(ch, "%-40s %6.0f %6.0f %6.0f\n", outbuf.c_str(),
                (target->vx - ctx.ship->vx),
                (target->vy - ctx.ship->vy),
                (target->vz - ctx.ship->vz));              
            }
            else if ( abs( (int) ( target->vx - ctx.ship->vx )) < 100*(ctx.ship->mod->sensor+10)*((target->shipclass == SHIP_DEBRIS ? 2 : target->shipclass)+3) &&
            abs( (int) ( target->vy - ctx.ship->vy )) < 100*(ctx.ship->mod->sensor+10)*((target->shipclass == SHIP_DEBRIS ? 2 : target->shipclass)+3) &&
            abs( (int) ( target->vz - ctx.ship->vz )) < 100*(ctx.ship->mod->sensor+10)*((target->shipclass == SHIP_DEBRIS ? 2 : target->shipclass)+3) )
            {
              if ( target->shipclass == FIGHTER_SHIP )
                outbuf = "A small metallic mass";
              if ( target->shipclass == MIDSIZE_SHIP )
                outbuf = "A goodsize metallic mass";
              if ( target->shipclass == SHIP_DEBRIS )
                outbuf = "scattered metallic reflections";
              else if ( target->shipclass >= CAPITAL_SHIP )
                outbuf = "A huge metallic mass";
              ch_printf(ch, "%-40s %6.0f %6.0f %6.0f\n", outbuf.c_str(),
                (target->vx - ctx.ship->vx),
                (target->vy - ctx.ship->vy),
                (target->vz - ctx.ship->vz));                
            }


        }

    }
    ch_printf(ch,"\n");
    for ( missile = first_missile; missile; missile = missile->next )
    {

        if ( abs( (int) ( missile->mx - ctx.ship->vx )) < 50*(ctx.ship->mod->sensor+10)*2 &&
        abs( (int) ( missile->my - ctx.ship->vy )) < 50*(ctx.ship->mod->sensor+10)*2 &&
        abs( (int) ( missile->mz - ctx.ship->vz )) < 50*(ctx.ship->mod->sensor+10)*2 )
        {
          ch_printf(ch, "%-40s %6.0f %6.0f %6.0f\n",
              missile->missiletype == CONCUSSION_MISSILE ? "A Concussion missile" :
            ( missile->missiletype ==  PROTON_TORPEDO ? "A Torpedo" :
            ( missile->missiletype ==  HEAVY_ROCKET ? "A Heavy Rocket" : "A Heavy Bomb" ) ),
            (missile->mx - ctx.ship->vx),
            (missile->my - ctx.ship->vy),
            (missile->mz - ctx.ship->vz));
        }
    }

    ch_printf(ch, "\n&WYour Coordinates: %.0f %.0f %.0f\n" ,
              ctx.ship->vx , ctx.ship->vy, ctx.ship->vz);
      
            
    learn_from_success( ch, gsn_navigation );
  
}

void do_autotrack( CHAR_DATA *ch, char *argument )
{
    scmd_data ctx;
    ctx.rest = argument;

    if (!space_require_ship_from_cockpit( ch , ctx )) return;    
    int chance;
  
    if (!space_require_ship_spaceship( ch , ctx )) return;
    if (!space_require_ship_notplatform( ch , ctx )) return;
    if (!space_require_ship_not_capital( ch , ctx )) return;
    if (!space_require_ship_not_landed_or_docked( ch , ctx )) return;
    if (!space_require_ship_from_pilotseat( ch , ctx )) return;   
    if (!space_require_ship_autopilot_off( ch , ctx )) return;

    chance = IS_NPC(ch) ? ch->top_level
             : (int)  (ch->pcdata->learned[gsn_shipsystems]) ;
    if ( number_percent( ) > chance )
    {
        send_to_char("&RYour not sure which switch to flip.\n",ch);
        learn_from_failure( ch, gsn_shipsystems );
      return;	
    }
   
    act( AT_PLAIN, "$n flips a switch on the control panel.", ch,
         NULL, ctx.rest , TO_ROOM );
    if (ctx.ship->autotrack)
    {
      ctx.ship->autotrack = FALSE;
      echo_to_cockpit( AT_YELLOW , ctx.ship, "Autotracking off.");
    }
    else
    {
        ctx.ship->autotrack = TRUE;
        echo_to_cockpit( AT_YELLOW , ctx.ship, "Autotracking on.");
    }
   
    learn_from_success( ch, gsn_shipsystems );
        
}

void do_findserin( CHAR_DATA *ch, char *argument )
{
    bool ch_comlink = FALSE;
    OBJ_DATA *obj;
    int next_planet, bus, stopnum = 1;
    std::string arg;
    std::string arg2;
    std::string argstr = argument;
    argstr = one_argument( argstr, arg );
    arg2 = argstr;


    for ( obj = ch->last_carrying; obj; obj = obj->prev_content )
    {
        if (obj->pIndexData->item_type == ITEM_COMLINK)
        ch_comlink = TRUE;
    }

    if ( !ch_comlink && !IS_IMMORTAL(ch))
    {
        send_to_char( "You need a comlink to do that!\n", ch);
        return;
    }

    if ( arg.empty() )
    {
        ch_printf( ch, "&cList of Serins:  \n\n" );
        for( bus = 0; bus < MAX_BUS; bus++ )
                ch_printf( ch, "&C%s &c- ", serin[bus].name );
        return;
    }


    for( bus = 0; bus < MAX_BUS; bus++ )
        if( !str_cmp( serin[bus].name, arg ) )
          break;
    if( bus == MAX_BUS )
    {
        send_to_char( "No such serin!\n", ch);
        return;
    }

    if ( bus_pos < 7 && bus_pos > 1 )
        ch_printf( ch, "The %s is Currently docked at %s.\n", serin[bus].name, serin[bus].bus_stop[serin[bus].planetloc] );

    next_planet = serin[bus].planetloc;
    send_to_char( "Next stops: ", ch);
    if ( bus_pos <= 1 )
        ch_printf( ch, "%s  ", serin[bus].bus_stop[next_planet] );

    while ( serin[bus].bus_vnum[stopnum] != serin[bus].bus_vnum[0] )
        stopnum++;
    stopnum--;
    for ( ;; )
    {
        next_planet++;
        if ( next_planet > stopnum )
            next_planet = 0;
        ch_printf( ch, "%s  ", serin[bus].bus_stop[next_planet] );
        if( serin[bus].planetloc == next_planet )
            break;
    }

    ch_printf( ch, "\n\n" );

}

void do_pluogus( CHAR_DATA *ch, char *argument )
{
  do_findserin( ch, "pluogus" );
  return;
}

void do_fly( CHAR_DATA *ch, char *argument )
{
    send_to_char( "Not implemented yet!\n", ch);
}

void do_drive( CHAR_DATA *ch, char *argument )
{
    scmd_data ctx;
    ctx.rest = argument;


    int dir;
    SHIP_DATA *target;
    
    ctx.rest = one_argument( ctx.rest, ctx.arg1 );
    ctx.arg2 = ctx.rest;

    if (!space_require_ship_from_cockpit( ch , ctx, false )) 
    {
        send_to_char("&RYou must be in the drivers seat of a land vehicle to do that!\n",ch);
        return;
    }

    if (space_require_ship_spaceship( ch , ctx ))
    {
        send_to_char("&RThis isn't a land vehicle!\n",ch);
        return;
    }
        
    if (!space_require_ship_not_disabled( ch , ctx )) return;
    	        
    if (!space_require_ship_energy( ch , ctx , 1 )) return;  
        
    if ( !str_cmp( ctx.arg1, "in" ))
    {
        target = ship_in_room( ctx.ship->in_room , ctx.arg2 );
        if ( !space_require_target_hangar_valid( ch , ctx )) return;

        if ( BV_IS_SET( target->in_room->room_flags, ROOM_INDOORS ) 
        || target->in_room->sector_type == SECT_INSIDE ) 
        {
            send_to_char( "You can't drive indoors!\n", ch );
            return;
        } 

        send_to_char("You drive the vehicle into the bay.\n", ch);           
        echo_to_room( AT_GREY,  ctx.ship->in_room, str_printf("%s drives into %s.", ctx.ship->name, target->name));
        
        transship(ctx.ship, target->hanger);
        
        echo_to_room( AT_GREY, ctx.ship->in_room, str_printf("%s drives into the bay", ctx.ship->name));
        learn_from_success( ch, gsn_speeders );
        return;
    }
           
    if ( !str_cmp( ctx.arg1, "out" ))
    {
        target = ship_from_hanger(ctx.ship->game, ctx.ship->in_room->vnum);
        if (!target)
        {
            send_to_char("You have to be in a ship's hanger to drive out of one.\n", ch);
            return;
        }
        
        if ( target->spaceobject != NULL )
        {
            send_to_char("The ship must be landed before you drive out of its hanger!\n", ch);
            return;
        }
        
        if (!target->bayopen)
        {
            send_to_char("The ship's bay doors must be open.\n", ch);
            return;
        }
        
        if ( BV_IS_SET( target->in_room->room_flags, ROOM_INDOORS ) 
        || target->in_room->sector_type == SECT_INSIDE ) 
        {
            send_to_char( "You can't drive indoors!\n", ch );
            return;
        }
        
        send_to_char("You drive the vehicle out of the bay.\n", ch);           
        echo_to_room( AT_GREY,  ctx.ship->in_room, str_printf("%s drives out of the ship.", ctx.ship->name));
        
        transship(ctx.ship, target->in_room->vnum);
        
        echo_to_room( AT_GREY, ctx.ship->in_room, str_printf("%s drives out of %s", ctx.ship->name, target->name));
        learn_from_success( ch, gsn_speeders );
        return;
    }
          
          

    if ( ( dir = get_door( ctx.arg1 ) ) == -1 )
    {
          send_to_char( "Usage: drive <direction>\n", ch );
          return;
    }
    
    drive_ship( ch, ctx.ship, get_exit(get_room_index(ctx.ship->location), dir), 0 );

}

ch_ret drive_ship( CHAR_DATA *ch, SHIP_DATA *ship, EXIT_DATA  *pexit , int fall )
{
    ROOM_INDEX_DATA *in_room;
    ROOM_INDEX_DATA *to_room;
  //ROOM_INDEX_DATA *from_room;
    ROOM_INDEX_DATA *original;
    std::string buf;
    std::string txt;
    std::string dtxt;
    ch_ret retcode;
    sh_int door, chance;//, distance;
    bool drunk = FALSE;
    CHAR_DATA * rch;
    CHAR_DATA * next_rch;
    

    if ( !IS_NPC( ch ) )
      if ( IS_DRUNK( ch, 2 ) && ( ch->position != POS_SHOVE )
      && ( ch->position != POS_DRAG ) )
        drunk = TRUE;

    if ( drunk && !fall )
    {
        door = number_door();
        pexit = get_exit( get_room_index(ship->location), door );
    }

    //if ( pexit )
    //{
	  //    log_printf( "drive_ship: %s to door %d", ch->name, pexit->vdir );
    //}

    retcode = rNONE;

    in_room = get_room_index(ship->location);
    if ( !pexit || (to_room = pexit->to_room) == NULL )
    {
        if ( drunk )
            send_to_char( "You drive into a wall in your drunken state.\n", ch );
        else
            send_to_char( "Alas, you cannot go that way.\n", ch );
        return rNONE;
    }

    door = pexit->vdir;

    if ( IS_SET( pexit->exit_info, EX_WINDOW )
    &&  !IS_SET( pexit->exit_info, EX_ISDOOR ) )
    {
        send_to_char( "Alas, you cannot go that way.\n", ch );
        return rNONE;
    }

    if (  IS_SET(pexit->exit_info, EX_PORTAL) 
       && IS_NPC(ch) )
    {
        act( AT_PLAIN, "Mobs can't use portals.", ch, NULL, NULL, TO_CHAR );
	      return rNONE;
    }

    if ( IS_SET(pexit->exit_info, EX_NOMOB)
	  && IS_NPC(ch) )
    {
        act( AT_PLAIN, "Mobs can't enter there.", ch, NULL, NULL, TO_CHAR );
        return rNONE;
    }

    if ( IS_SET(pexit->exit_info, EX_CLOSED)
    && (IS_SET(pexit->exit_info, EX_NOPASSDOOR)) )
    {
        if ( !IS_SET( pexit->exit_info, EX_SECRET )
        &&   !IS_SET( pexit->exit_info, EX_DIG ) )
        {
            if ( drunk )
            {
              act( AT_PLAIN, "$n drives into the $d in $s drunken state.", ch,
            NULL, pexit->keyword, TO_ROOM );
              act( AT_PLAIN, "You drive into the $d in your drunken state.", ch,
            NULL, pexit->keyword, TO_CHAR ); 
            }
            else
              act( AT_PLAIN, "The $d is closed.", ch, NULL, pexit->keyword, TO_CHAR );
        }
        else
        {
            if ( drunk )
              send_to_char( "You hit a wall in your drunken state.\n", ch );
            else
              send_to_char( "Alas, you cannot go that way.\n", ch );
        }

        return rNONE;
    }

/*
    if ( distance > 1 )
	if ( (to_room=generate_exit(in_room, &pexit)) == NULL )
	    send_to_char( "Alas, you cannot go that way.\n", ch );
*/
    if ( room_is_private( ch, to_room ) )
    {
	send_to_char( "That room is private right now.\n", ch );
	return rNONE;
    }

    if ( !IS_IMMORTAL(ch)
    &&  !IS_NPC(ch)
    &&  ch->in_room->area != to_room->area )
    {
	if ( ch->top_level < to_room->area->low_hard_range )
	{
	    set_char_color( AT_TELL, ch );
	    switch( to_room->area->low_hard_range - ch->top_level )
	    {
		case 1:
		  send_to_char( "A voice in your mind says, 'You are nearly ready to go that way...'", ch );
		  break;
		case 2:
		  send_to_char( "A voice in your mind says, 'Soon you shall be ready to travel down this path... soon.'", ch );
		  break;
		case 3:
		  send_to_char( "A voice in your mind says, 'You are not ready to go down that path... yet.'.\n", ch);
		  break;
		default:
		  send_to_char( "A voice in your mind says, 'You are not ready to go down that path.'.\n", ch);
	    }
	    return rNONE;
	}
	else
	if ( ch->top_level > to_room->area->hi_hard_range )
	{
	    set_char_color( AT_TELL, ch );
	    send_to_char( "A voice in your mind says, 'There is nothing more for you down that path.'", ch );
	    return rNONE;
	}          
    }

    if ( !fall )
    {
        if ( BV_IS_SET( to_room->room_flags, ROOM_INDOORS ) 
        || BV_IS_SET( to_room->room_flags, ROOM_SPACECRAFT )  
        || to_room->sector_type == SECT_INSIDE ) 
	{
		send_to_char( "You can't drive indoors!\n", ch );
		return rNONE;
	}
        
        if ( BV_IS_SET( to_room->room_flags, ROOM_NO_DRIVING ) ) 
	{
		send_to_char( "You can't take a vehicle through there!\n", ch );
		return rNONE;
	}

	if ( in_room->sector_type == SECT_AIR
	||   to_room->sector_type == SECT_AIR
	||   IS_SET( pexit->exit_info, EX_FLY ) )
	{
            if ( ship->shipclass > CLOUD_CAR )
	    {
		send_to_char( "You'd need to fly to go there.\n", ch );
		return rNONE;
	    }
	}

	if ( in_room->sector_type == SECT_WATER_NOSWIM
	||   to_room->sector_type == SECT_WATER_NOSWIM 
	||   to_room->sector_type == SECT_WATER_SWIM
	||   to_room->sector_type == SECT_UNDERWATER
	||   to_room->sector_type == SECT_OCEANFLOOR )
	{

	    if ( ship->shipclass != OCEAN_SHIP )
	    {
		send_to_char( "You'd need a boat to go there.\n", ch );
		return rNONE;
	    }
	    	    
	}

	if ( IS_SET( pexit->exit_info, EX_CLIMB ) )
	{

	    if ( ship->shipclass < CLOUD_CAR )
	    {
		send_to_char( "You need to fly or climb to get up there.\n", ch );
		return rNONE;
	    }
	}

    }

    if ( to_room->tunnel > 0 )
    {
        CHAR_DATA *ctmp;
        int count = 0;
        
        for ( ctmp = to_room->first_person; ctmp; ctmp = ctmp->next_in_room )
          if ( ++count >= to_room->tunnel )
          {
              send_to_char( "There is no room for you in there.\n", ch );
              return rNONE;
          }
    }

    if ( fall )
      txt = "falls";
    else
    {
      if ( txt.empty() )
      {
          if (  ship->shipclass < OCEAN_SHIP )
              txt = "fly";
          else
          if ( ship->shipclass == OCEAN_SHIP  )
          {
              txt = "float";
          }
          else
          if ( ship->shipclass > OCEAN_SHIP  )
          {
              txt = "drive";
          }
      }
    }

    chance = IS_NPC(ch) ? ch->top_level
      : (int)  (ch->pcdata->learned[gsn_speeders]) ;
    if ( number_percent( ) > chance )
    {
        send_to_char("&RYou can't figure out which switch it is.\n",ch);
        learn_from_failure( ch, gsn_speeders );
      return retcode;	
    }


    buf = str_printf( "$n %ss the vehicle $T.", txt );
    act( AT_ACTION, buf, ch, NULL, dir_name[door], TO_ROOM );
    buf = str_printf("You %s the vehicle $T.", txt );
    act( AT_ACTION, buf, ch, NULL, dir_name[door], TO_CHAR );
    echo_to_room( AT_ACTION , get_room_index(ship->location) , str_printf("%s %ss %s.", ship->name, txt, dir_name[door]) );

    extract_ship( ship );
    ship_to_room(ship, to_room->vnum );
    
    ship->location = to_room->vnum;
    ship->lastdoc = ship->location;
  
    if ( fall )
      txt = "falls";
    else
	  if (  ship->shipclass < OCEAN_SHIP )
	      txt = "flies in";
	  else
	  if ( ship->shipclass == OCEAN_SHIP  )
	  {
	      txt = "floats in";
	  }
	  else
	  if ( ship->shipclass > OCEAN_SHIP  )
	  {
	      txt = "drives in";
	  }

      switch( door )
      {
      default: dtxt = "somewhere";	break;
      case 0:  dtxt = "the south";	break;
      case 1:  dtxt = "the west";	break;
      case 2:  dtxt = "the north";	break;
      case 3:  dtxt = "the east";	break;
      case 4:  dtxt = "below";		break;
      case 5:  dtxt = "above";		break;
      case 6:  dtxt = "the south-west";	break;
      case 7:  dtxt = "the south-east";	break;
      case 8:  dtxt = "the north-west";	break;
      case 9:  dtxt = "the north-east";	break;
      }

    echo_to_room( AT_ACTION , get_room_index(ship->location) , str_printf("%s %s from %s.", ship->name, txt.c_str(), dtxt.c_str()) );
    
    for ( rch = ch->in_room->last_person ; rch ; rch = next_rch )
    { 
        next_rch = rch->prev_in_room;
        original = rch->in_room;
        char_from_room( rch );
        char_to_room( rch, to_room );
        do_look( rch, "auto" );
        char_from_room( rch );
        char_to_room( rch, original );
    }
    
    learn_from_success( ch, gsn_speeders );
    return retcode;

}

void do_bomb( CHAR_DATA *ch, char *argument )
{        
        send_to_char( "Not implemented yet!\n", ch);
 }

void do_chaff( CHAR_DATA *ch, char *argument )
{
    int chance;
    scmd_data ctx;
    ctx.rest = argument;

    if (!space_require_ship_from_cockpit( ch , ctx )) return;    
    if (!space_require_ship_spaceship( ch , ctx )) return;
    if ( !space_require_ship_from_coseat( ch , ctx )) return;
    if (!space_require_ship_autopilot_off( ch , ctx )) return;
    if (!space_require_ship_not_hyperspace( ch , ctx )) return;
    if (!space_require_ship_not_landed( ch , ctx )) return;

    if (ctx.ship->chaff <= 0 )
    {
        send_to_char("&RYou don't have any chaff to release!\n",ch);
        return;
    }
    chance = IS_NPC(ch) ? ch->top_level
      : (int)  (ch->pcdata->learned[gsn_weaponsystems]) ;
    if ( number_percent( ) > chance )
    {
        send_to_char("&RYou can't figure out which switch it is.\n",ch);
        learn_from_failure( ch, gsn_weaponsystems );
      return;	
    }

    ctx.ship->chaff--;
    
    ctx.ship->chaff_released++;
        
    send_to_char( "You flip the chaff release switch.\n", ch);
    act( AT_PLAIN, "$n flips a switch on the control pannel", ch,
         NULL, ctx.rest , TO_ROOM );
    echo_to_cockpit( AT_YELLOW , ctx.ship , "A burst of chaff is released from the ship.");
	  
    learn_from_success( ch, gsn_weaponsystems );

}


void modtrainer( SHIP_DATA *ship, sh_int shipclass )
{
/*
  switch(shipclass)
  {
    case 0:
      ship->maxenergy = 1000;
      ship->lasers = 0;
      ship->ions = 0;
      ship->shield = 0;
      ship->maxhull = 0;
      ship->realspeed = 1;
      ship->manuever = 0;
      ship->hyperspeed = 0;
      ship->maxmissiles = 0;
      ship->maxtorpedos = 0;
      ship->maxrockets = 0;
      ship->maxchaff = 0;
      break;
    case 1:
      ship->maxenergy = 5000;
      ship->lasers = 2;
      ship->ions = 2;
      ship->shield = 100;
      ship->maxhull = 1000;
      ship->realspeed = 120;
      ship->manuever = 120;
      ship->hyperspeed = 150;
      ship->maxmissiles = 5;
      ship->maxtorpedos = 2;
      ship->maxrockets = 1;
      ship->maxchaff = 1;
      break;
    case 2:
      ship->maxenergy = 10000;
      ship->lasers = 3;
      ship->ions = 3;
      ship->shield = 300;
      ship->maxhull = 2500;
      ship->realspeed = 90;
      ship->manuever = 60;
      ship->hyperspeed = 200;
      ship->maxmissiles = 8;
      ship->maxtorpedos = 3;
      ship->maxrockets = 2;
      ship->maxchaff = 8;
      break;
    case 3:
      ship->maxenergy = 20000;
      ship->lasers = 4;
      ship->ions = 4;
      ship->shield = 500;
      ship->maxhull = 10000;
      ship->realspeed = 30;
      ship->manuever = 0;
      ship->hyperspeed = 150;
      ship->maxmissiles = 25;
      ship->maxtorpedos = 10;
      ship->maxrockets = 5;
      ship->maxchaff = 50;
      break;
  }
*/  
  return;
}    
    
      

bool autofly( SHIP_DATA *ship )
{
 
     if (!ship)
        return FALSE;
     
     if ( ship->type == MOB_SHIP )
        return TRUE;
     
     if ( ship->autopilot )
        return TRUE;
     
     return FALSE;   

}

void makedebris( SHIP_DATA *ship )
{
  SHIP_DATA *debris;
  std::string buf;

  if ( ship->shipclass == SHIP_DEBRIS )
    return;
 
  CREATE( debris, SHIP_DATA, 1 );
  debris->game = ship->game;

  LINK( debris, first_ship, last_ship, next, prev );

  debris->owner 	= STRALLOC( "" );
  debris->copilot       = STRALLOC( "" );
  debris->pilot         = STRALLOC( "" );
  debris->home          = STRALLOC( "" );
  debris->type		= SHIP_CIVILIAN; 
  if( ship->type != MOB_SHIP )
    debris->type          = ship->type;
  debris->shipclass         = SHIP_DEBRIS;
  debris->lasers        = ship->mod->lasers  ;
  debris->missiles   = ship->missiles  ;
  debris->rockets        = ship->rockets  ;
  debris->torpedos        = ship->torpedos  ;
  debris->maxshield        = ship->mod->maxshield  ;
  debris->maxhull        = ship->mod->maxhull  ;
  debris->maxenergy        = ship->mod->maxenergy  ;
  debris->hyperspeed        = ship->hyperspeed  ;
  debris->chaff        = ship->chaff  ;
  debris->realspeed        = ship->mod->realspeed  ;
  debris->currspeed        = ship->currspeed  ;
  debris->manuever        = ship->mod->manuever  ;

  debris->spaceobject = NULL;
  debris->energy = 0;
  debris->hull = ship->mod->maxhull;
  debris->in_room=NULL;
  debris->next_in_room=NULL;
  debris->prev_in_room=NULL;
  debris->currjump=NULL;
  debris->target0=NULL;
  debris->autopilot = FALSE;
 
  update_ship_modules( debris );
 
  add_random_modules( debris, ship );
 
  buf = "Debris of a ";
  buf += ship->name;
  debris->name		= STRALLOC( "Debris" );
  debris->personalname		= STRALLOC( "Debris" );
  debris->description	= STRALLOC( buf );
  
  ship_to_spaceobject( debris, ship->spaceobject );
  debris->vx = ship->vx;
  debris->vy = ship->vy;
  debris->vz = ship->vz;
  debris->hx = ship->hx;
  debris->hy = ship->hy;
  debris->hz = ship->hz;
  
  return;
  
}

void unlink_ship_rooms( GameContext *game, SHIP_DATA *ship )
{
    if ( !ship )
        return;

    for ( int vnum = ship->get_firstroom(); vnum <= ship->get_lastroom(); ++vnum )
    {
        ROOM_INDEX_DATA *room = get_room_index( vnum );
        if ( room && room->ship == ship )
            room->ship = nullptr;
    }
}

void shipdelete(SHIP_DATA * ship, bool shiplist) {

  std::string buf, buf2;
  GameContext *game = ship->game;
  
  buf = str_printf("%s%s", SHIP_DIR, ship->filename);
  buf2 = str_printf("%s%s", BACKUPSHIP_DIR, ship->filename);
  
  rename( buf.c_str(), buf2.c_str() );

  unlink_ship_rooms( ship->game, ship );

  UNLINK (ship, first_ship, last_ship, next, prev);
  extract_ship(ship);
  STRFREE(ship->owner);
  STRFREE(ship->copilot);
  STRFREE(ship->pilot);
  STRFREE(ship->home);

  STRFREE(ship->name);
  STRFREE(ship->personalname);
  STRFREE(ship->description);
  STRFREE(ship->templatestring);

  if(ship->cockpitroom) {
    delete_room(ship->cockpitroom);
    ship->cockpitroom = NULL;
  }
  
  DISPOSE( ship->mod );
  DISPOSE( ship );
 
  if( shiplist )
    write_ship_list( game );
 
  return; 
}

void storeship( SHIP_DATA *ship )
{
  transship( ship, RENTALSTORAGEROOM );
  
  return;
}

void dumpship( SHIP_DATA *ship, int destination )
{
  CHAR_DATA *ch;
  OBJ_DATA *obj;
  ROOM_INDEX_DATA *room;
  ROOM_INDEX_DATA *toroom;

  if( !( room = ship->cockpitroom ) )
    return;

  if( !( toroom = get_room_index( destination ) ) )
    return;
    
      ch = room->first_person;
      while (ch) {
        char_from_room (ch);
        char_to_room (ch, toroom);
  send_to_char( "You exit the remains of your escape pod and enter your salvager's ship.", ch );
  ch = room->first_person;
      }

  obj = room->first_content;
  while ( obj ) {
    obj_from_room( obj );
    obj_to_room( obj, toroom );
    obj = room->first_content;
  }

//storeship( ship );
  return; 
}

SHIP_DATA *create_virtual_ship( GameContext *game, SHIP_DATA *shiptemplate )
{
  SHIP_DATA *ship;
  SHIP_MOD_DATA *ship_mod;
  ROOM_INDEX_DATA *cockpitroom;
  ROOM_INDEX_DATA *templateroom;
  std::string buf;
  int roomvnum;

  if ( !shiptemplate )
    return NULL;

  CREATE (ship, SHIP_DATA, 1);
  ship->game = game;
  roomvnum    = 100000 + (currrentalvnum ^ 65535);
  currrentalvnum++;
  templateroom = get_room_index(shiptemplate->cockpit);
 
  LINK (ship, first_ship, last_ship, next, prev);

  ship->owner         = STRALLOC ("");
  ship->copilot       = STRALLOC ("");
  ship->pilot         = STRALLOC ("");
  ship->home          = STRALLOC ("");
  ship->type          = SHIP_CIVILIAN;
  ship->shipclass         = shiptemplate->shipclass;
  ship->lasers        = shiptemplate->lasers;
  ship->missiles      = shiptemplate->missiles;
  ship->rockets       = shiptemplate->rockets;
  ship->torpedos      = shiptemplate->torpedos;
  ship->maxshield     = shiptemplate->maxshield;
  ship->maxhull       = shiptemplate->maxhull;
  ship->maxenergy     = shiptemplate->maxenergy;
  ship->hyperspeed    = shiptemplate->hyperspeed;
  ship->chaff         = shiptemplate->chaff;
  ship->realspeed     = shiptemplate->realspeed;
  ship->currspeed     = shiptemplate->currspeed;
  ship->manuever      = shiptemplate->manuever;

  ship->maxextmodules = shiptemplate->maxextmodules;
  ship->maxintmodules = shiptemplate->maxintmodules;

  ship->entrance      = roomvnum;
  ship->engineroom    = roomvnum;
  ship->set_firstroom( roomvnum );
  ship->set_lastroom( roomvnum );
  ship->navseat       = roomvnum;
  ship->pilotseat     = roomvnum;
  ship->coseat        = roomvnum;
  ship->gunseat       = roomvnum;
  
  ship->shipstate     = SHIP_LANDED;
  ship->docking       = SHIP_READY;
  ship->statei0       = LASER_READY;
  ship->statet0       = LASER_READY;
  ship->statettractor = SHIP_READY;
  ship->statetdocking = SHIP_READY;
  ship->missilestate  = MISSILE_READY;

  ship->spaceobject   = NULL;
  ship->energy        = shiptemplate->energy;
  ship->hull          = shiptemplate->hull;
  ship->in_room       = get_room_index(45);
  ship->next_in_room  = NULL;
  ship->prev_in_room  = NULL;
  ship->currjump      = NULL;
  ship->target0       = NULL;
  ship->tractoredby   = NULL;
  ship->tractored     = NULL;
  ship->docked        = NULL;
  ship->autopilot     = FALSE;
  
  CREATE( ship_mod, SHIP_MOD_DATA, 1 );
  ship->mod = ship_mod;
  update_ship_modules(ship);
  
  ship->name          = STRALLOC (shiptemplate->name);
  buf = str_printf("%d",roomvnum);
  ship->personalname  = STRALLOC (buf.c_str());
  ship->description   = STRALLOC (shiptemplate->description);

  cockpitroom = make_room( game, roomvnum );

  cockpitroom->area    = templateroom->area;
  cockpitroom->sector_type = templateroom->sector_type;
  cockpitroom->room_flags  = templateroom->room_flags;
  cockpitroom->name = STRALLOC( templateroom->name );
  cockpitroom->description = STRALLOC( templateroom->description );
  
  ship->cockpitroom = cockpitroom;
  
  transship( ship, 45 );

  return ship;
}

void create_rental( CHAR_DATA *ch, SHIP_DATA *shiptemplate )
{
  SHIP_DATA *ship;
  ROOM_INDEX_DATA *room;
  
  if ( ch->gold < ( get_ship_value( shiptemplate )/100 ) )
  {
    ch_printf( ch, "You do not currently have the %d needed to rent this ship./n/r", get_ship_value( shiptemplate ) / 100 );
    return;
  }

  ch_printf( ch, "You rent the %s for %d credits.\n", shiptemplate->name, (get_ship_value(shiptemplate)/100) );

  if( (room = get_room_index(RENTALSTORAGEROOM)) != NULL )
  {
    for( ship = first_ship; ship; ship = ship->next )
      if( ship->location == room->vnum && !str_cmp( ship->name, shiptemplate->name ) )
        break;
  }
  else
  {
    ch_printf( ch, "Rental storage room not found.  Contact DV!/n/r" );
    return;
  }

  if ( !ship && !(ship = create_virtual_ship( ch->game, shiptemplate )) )
    send_to_char( "ERROR! Please contact your administrator.\n", ch );

  transship( ship, ch->in_room->vnum );

  STRFREE(ship->owner);
  ship->owner = STRALLOC( "Public" );
  
  ch->gold -= get_ship_value( shiptemplate ) / 100;
  
  return;
}

void do_rent( CHAR_DATA *ch, char *argument )
{
  SHIP_DATA *ship;
  sh_int rentnum;
  
  if ( !argument || argument[0] == '\0' )
  {
    for( rentnum = 0; rentnum < MAX_RENTALS; rentnum++ )
    {
      ship = ship_from_cockpit( ch->game, rentals[rentnum].templatevnum );
      if ( !ship ) 
        ch_printf( ch, "[%d] ERROR-Non-existent ship\n", rentnum );
      else
        ch_printf( ch, "[%d] %.50s %d to rent.\n", rentnum, ship->name, (get_ship_value(ship)/100) );
//    return;
    }
    return;
  }

  if ( !BV_IS_SET( ch->in_room->room_flags, ROOM_CAN_LAND ) )
  {
    send_to_char( "You can't rent ships here\n", ch );
    return;
  }

  for ( rentnum = 0; rentnum < MAX_RENTALS; rentnum++ )
  {
    ship = ship_from_cockpit( ch->game, rentals[rentnum]. templatevnum );
    if ( ship && !str_cmp( argument, ship->name ) )
    {
      create_rental(ch, ship);
      return;
    }
  }
  for ( rentnum = 0; rentnum < MAX_RENTALS; rentnum++ )
  {
    ship = ship_from_cockpit( ch->game, rentals[rentnum]. templatevnum );
    if ( ship && ship->name && (ship->name[0] != '\0') && nifty_is_name_prefix( argument, ship->name ) )
    {
      create_rental(ch, ship);
      return;
    }
  }
  ch_printf( ch, "There is no such rental choice. (%s)", argument );
  return;
}


/* Generic Pilot Command To use as template

void do_hmm( CHAR_DATA *ch, char *argument )
{
    scmd_data ctx;                                              // Context struct used for space commands.  Contains ship, target, and argument variables 
    ctx.rest = argument;                                        // that can be used for checks and command execution.  Also used for learning and skill checks.

    int chance;

    ctx.rest = one_argument(ctx.rest, ctx.arg1);
    ctx.rest = one_argument(ctx.rest, ctx.arg2);
    ctx.rest = one_argument(ctx.rest, ctx.arg3);
    ctx.rest = one_argument(ctx.rest, ctx.arg4);
    ctx.rest = one_argument(ctx.rest, ctx.arg5);

    switch( ch->substate )
    { 
    	default:

    if (!space_require_ship_from_cockpit( ch , ctx )) return;     // Requires that you are in a ship's control seat
    if (!space_require_ship_spaceship( ch , ctx )) return;        // Requires that you are in a spaceship, (false if in a land vehicle)
    if (!space_require_ship_movement( ch , ctx )) return;         // Requires that the ship can move - not a platform, not docked to a ship, also at least has 1 energy
    if (!space_require_ship_from_pilotseat( ch , ctx )) return;   // Requires that you are in the pilot's seat
    if (!space_require_ship_autopilot_off( ch , ctx )) return;    // Requires that the ship's autopilot is off
    if (!space_require_ship_not_disabled( ch , ctx )) return;     // Requires that the ship is not disabled
    if (!space_require_ship_not_hyperspace( ch , ctx )) return;  // Requires that the ship is not in hyperspace
    if (!space_require_ship_not_landed( ch , ctx )) return;      // Requires that the ship is not landed
    if (!space_require_ship_not_disabled( ch , ctx )) return;    // Requires that the ship is not disabled
    if (!space_require_ship_ready( ch , ctx )) return;           // Requires that the ship is ready-state (not in the middle of manuevering)
    if (!space_require_ship_ownership( ch , ctx )) return;       // Requires that the ship is owned by the character
    if (!space_require_ship_tractored( ch , ctx )) return;       // Requires that the ship is not being tractored
    if (!space_requires_finish_launching( ch , ctx )) return;   // Requires that the ship has finished launching
    if (!space_requires_in_system( ch , ctx )) return;           // Requires that the ship is in a system (not in hyperspace, not in a starport, etc)

    if (!space_require_ship_notplatform( ch , ctx )) return;    // Requires that the ship is not a platform 
    if (!space_require_ship_not_capital( ch , ctx )) return;    // Requires that the ship is not a capital ship (if the command is something that only fighters and midships should be able to do)

    if ( !space_require_ship_from_turret( ch , ctx )) return;   // Requires that you are in a turret seat
    if ( !space_require_ship_from_engineroom( ch , ctx )) return; // Requires that you are in the engineroom
    if ( !space_require_ship_from_coseat( ch , ctx )) return;   // Requires that you are in a co-pilot seat
    if ( !space_require_ship_from_navseat( ch , ctx )) return;  // Requires that you are in a nav seat

    if ( !space_require_target_hangar_valid( ch , ctx )) return;    // Requires that the target not NULL, not yourself, has a hangar, has room, and the bay is open
    
    if (!space_require_ship_energy( ch , ctx , required_energy )) return;  // Requires that the ship has at least the required_energy amount of energy

    chance = space_chance_by_shipclass( ch, ctx.ship ); // Sets chance based on the ch's skill with the shipclass (fighter, midship, capital)
    if ( number_percent( ) < chance )
    		{
            send_to_char( "&G\n", ch);
            act( AT_PLAIN, "$n does  ...", ch,
            NULL, ctx.rest , TO_ROOM );
            echo_to_room( AT_YELLOW , get_room_index(ship->cockpit) , "");
            add_timer ( ch , TIMER_DO_FUN , 1 , do_hmm , 1 );
            ch->dest_buf = str_dup(ctx.arg1);
            return;
        }
        send_to_char("&RYou fail to work the controls properly.\n",ch);
        space_learn_from_failure( ch, ctx.ship );                           // Learn from failure based on shipclass (fighter, midship, capital)
    	   	return;	
    	
    	case 1:
    		if ( !ch->dest_buf )
    		   return;
    		SPRINTF(ctx.arg1, ch->dest_buf);
    		STR_DISPOSE( ch->dest_buf);
    		break;
    		
    	case SUB_TIMER_DO_ABORT:
    		STR_DISPOSE( ch->dest_buf );
    		ch->substate = SUB_NONE;
        if (!space_require_ship_from_cockpit( ch , ctx )) return;                            
    	  send_to_char("&Raborted.\n", ch);
    	  echo_to_room( AT_YELLOW , get_room_index(ctx.ship->cockpit) , "");
    		if (ctx.ship->shipstate != SHIP_DISABLED)
    		   space_set_shipstate(ctx.ship, SHIP_READY);
    		return;
    }
    
    ch->substate = SUB_NONE;
    
    if (!space_require_ship_from_cockpit( ch , ctx )) return;                            

    send_to_char( "&G\n", ch);
    act( AT_PLAIN, "$n does  ...", ch,
         NULL, ctx.rest , TO_ROOM );
    echo_to_room( AT_YELLOW , get_room_index(ctx.ship->cockpit) , "");

    space_learn_from_success( ch, ctx.ship );    	                        // Learn from success based on shipclass (fighter, midship, capital)
    	return;
}

void do_hmm( CHAR_DATA *ch, char *argument )
{
    scmd_data ctx;                                              // Context struct used for space commands.  Contains ship, target, and argument variables 
    ctx.rest = argument;                                        // that can be used for checks and command execution.  Also used for learning and skill checks.

    int chance;

    ctx.rest = one_argument(ctx.rest, ctx.arg1);
    ctx.rest = one_argument(ctx.rest, ctx.arg2);
    ctx.rest = one_argument(ctx.rest, ctx.arg3);
    ctx.rest = one_argument(ctx.rest, ctx.arg4);
    ctx.rest = one_argument(ctx.rest, ctx.arg5);

    if (!space_require_ship_from_cockpit( ch , ctx )) return;     // Requires that you are in a ship's control seat
    if (!space_require_ship_spaceship( ch , ctx )) return;        // Requires that you are in a spaceship, (false if in a land vehicle)
    if (!space_require_ship_movement( ch , ctx )) return;         // Requires that the ship can move - not a platform, not docked to a ship, also at least has 1 energy
    if (!space_require_ship_from_pilotseat( ch , ctx )) return;   // Requires that you are in the pilot's seat
    if (!space_require_ship_autopilot_off( ch , ctx )) return;    // Requires that the ship's autopilot is off
    if (!space_require_ship_not_disabled( ch , ctx )) return;     // Requires that the ship is not disabled
    if (!space_require_ship_not_hyperspace( ch , ctx )) return;  // Requires that the ship is not in hyperspace
    if (!space_require_ship_not_landed( ch , ctx )) return;      // Requires that the ship is not landed
    if (!space_require_ship_not_disabled( ch , ctx )) return;    // Requires that the ship is not disabled
    if (!space_require_ship_ready( ch , ctx )) return;           // Requires that the ship is ready-state (not in the middle of manuevering)
    if (!space_require_ship_ownership( ch , ctx )) return;       // Requires that the ship is owned by the character
    if (!space_require_ship_tractored( ch , ctx )) return;       // Requires that the ship is not being tractored
    if (!space_requires_finish_launching( ch , ctx )) return;   // Requires that the ship has finished launching
    if (!space_requires_in_system( ch , ctx )) return;           // Requires that the ship is in a system (not in hyperspace, not in a starport, etc)

    if (!space_require_ship_notplatform( ch , ctx )) return;    // Requires that the ship is not a platform 
    if (!space_require_ship_not_capital( ch , ctx )) return;    // Requires that the ship is not a capital ship (if the command is something that only fighters and midships should be able to do)

    if ( !space_require_ship_from_turret( ch , ctx )) return;   // Requires that you are in a turret seat
    if ( !space_require_ship_from_engineroom( ch , ctx )) return; // Requires that you are in the engineroom
    if ( !space_require_ship_from_coseat( ch , ctx )) return;   // Requires that you are in a co-pilot seat
    if ( !space_require_ship_from_navseat( ch , ctx )) return;  // Requires that you are in a nav seat

    if ( !space_require_target_hangar_valid( ch , ctx )) return;    // Requires that the target not NULL, not yourself, has a hangar, has room, and the bay is open
    
    if (!space_require_ship_energy( ch , ctx , required_energy )) return;  // Requires that the ship has at least the required_energy amount of energy

    chance = space_chance_by_shipclass( ch, ctx.ship ); // Sets chance based on the ch's skill with the shipclass (fighter, midship, capital)
    if ( number_percent( ) > chance )
    {
        send_to_char("&RYou fail to work the controls properly.\n",ch);
        space_learn_from_failure( ch, ctx.ship );                           // Learn from failure based on shipclass (fighter, midship, capital)
        return;	
    }
        
    send_to_char( "&G\n", ch);
    act( AT_PLAIN, "$n does  ...", ch,
         NULL, ctx.rest , TO_ROOM );
    echo_to_room( AT_YELLOW , get_room_index(ctx.ship->cockpit) , "");
	  
    space_learn_from_success( ch, ctx.ship );    	                        // Learn from success based on shipclass (fighter, midship, capital)
    return;    
    
}
*/

