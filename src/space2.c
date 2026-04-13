/***************************************************************************
*                           STAR WARS: RISE IN POWER                       *
*--------------------------------------------------------------------------*
* Star Wars: Rise in Power Code Additions to Star Wars Reality 1.0         *
* copyright (c) 1999 by the Coding Team at Star Wars: Rise in Power        *
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
*                               Space Module 2                             *
****************************************************************************/

#include "space2.h"

void affectshipcargo( SHIP_DATA *ship, int typeCargo, int amount );
bool candock( SHIP_DATA *ship );

bool	module_type_install		args	( ( OBJ_DATA *obj, SHIP_DATA *ship ) );
bool	module_type_install2		args	( ( int modtype, SHIP_DATA *ship, int modsize ) );

/* Flag data - structure for linking module "modification" to flags - DV 2/7/04 */
//First variable is size of array

int const modflags [MAXMODFLAG] = 
{
  4, ROOM_HOTEL, ROOM_FACTORY, ROOM_REFINERY, ROOM_CLANSTOREROOM
};

bool ship_was_in_range( SHIP_DATA *ship, SHIP_DATA *target )
{
  if (target && ship && target != ship )
    if ( abs( (int) ( target->ox - ship->vx )) < 100*(ship->mod->sensor+10)*((target->shipclass == SHIP_DEBRIS ? 2 : target->shipclass)+3) &&
         abs( (int) ( target->oy - ship->vy )) < 100*(ship->mod->sensor+10)*((target->shipclass == SHIP_DEBRIS ? 2 : target->shipclass)+3) &&
         abs( (int) ( target->oz - ship->vz )) < 100*(ship->mod->sensor+10)*((target->shipclass == SHIP_DEBRIS ? 2 : target->shipclass)+3) )
      return TRUE;
  return FALSE;
}	


void do_jumpvector( CHAR_DATA *ch, char *argument )
{ 
    int chance, num;
    float randnum, tx, ty, tz;
    SHIP_DATA *ship;
    SHIP_DATA *target;
    char buf[MAX_STRING_LENGTH];
   
    num = number_range( 1, 16 );
    randnum = 1.0/(float) num;
    if (  (ship = ship_from_cockpit(ch->game, ch->in_room->vnum))  == NULL )
    {
         send_to_char("&RYou must be in the cockpit, turret or engineroom of a ship to do that!\n",ch);
         return;
    }
    
    if ( !ship->spaceobject )
    {
    	send_to_char("&RYou have to be in realspace to do that!\n", ch);
    	return;
    }
    
    target = get_ship( ch->game, argument );
    if ( !target )
    {
	send_to_char( "No such ship.\n", ch );
	return;
    }
    
    if ( target == ship )
    {
	send_to_char( "You can figure out where you are going on your own.\n", ch );
	return;
    }
    
    if (!ship_was_in_range( ship, target ))
    {
    	send_to_char( "No log for that ship.\n", ch);
    	return;
    }
    if (target->shipstate == SHIP_LANDED)
    {
    	send_to_char( "No log for that ship.\n", ch);
    	return;
    }

    chance = IS_NPC(ch) ? ch->top_level
        : (int)  (ch->pcdata->learned[gsn_jumpvector]) ;
    if ( number_percent( ) > chance )
    {
        send_to_char("&RYou cant figure out the course vectors correctly.\n",ch);
        learn_from_failure( ch, gsn_shipsystems );
        return;	
    }	


    if( target->shipstate == SHIP_HYPERSPACE )
    {
      tx = (target->vx - target->ox)*randnum;
      ty = (target->vy - target->oy)*randnum;
      tz = (target->vz - target->oz)*randnum;
      
      send_to_char("After some deliberation, you figure out its projected course.\n", ch);
      SPRINTF(buf, "%s Heading: %.0f, %.0f, %.0f", target->name, tx, ty, tz );
      echo_to_cockpit( AT_BLOOD, ship , buf );
      learn_from_success( ch, gsn_jumpvector );
      return;
    }

      tx = (target->cx - target->ox)*randnum;
      ty = (target->cy - target->oy)*randnum;
      tz = (target->cz - target->oz)*randnum;

      send_to_char("After some deliberation, you figure out its projected course.\n", ch);
      SPRINTF(buf, "%s Heading: %.0f, %.0f, %.0f", target->name, tx, ty, tz  );
      echo_to_cockpit( AT_BLOOD, ship , buf );
      learn_from_success( ch, gsn_jumpvector );
      return;
    
}

void do_reload( CHAR_DATA *ch, char *argument )
{

  /* Reload code added by Darrik Vequir */


  char arg[MAX_INPUT_LENGTH];
  SHIP_DATA *ship;
  sh_int price = 0;


  SPRINTF( arg, "%s", argument );

  if (arg[0] == '\0')
  {
    send_to_char("&RYou need to specify a target!\n",ch);
    return;
  }

  if ( ( ship = ship_in_room( ch->in_room , argument ) ) == NULL )
  {
    act( AT_PLAIN, "I see no $T here.", ch, NULL, argument, TO_CHAR );
    return;
  }

  if (ship->shipstate == SHIP_DISABLED )
    price += 200;
  if ( ship->missilestate == MISSILE_DAMAGED )
    price += 100;
  if ( ship->statet0 == LASER_DAMAGED )
    price += 50;

  if ( ch->pcdata && ch->pcdata->clan && !str_cmp(ch->pcdata->clan->name,ship->owner) )
  {
    if ( ch->pcdata->clan->funds < price )
    {
      ch_printf(ch, "&R%s doesn't have enough funds to prepare this ship for launch.\n", ch->pcdata->clan->name );
      return;
    }

    ch->pcdata->clan->funds -= price;
    ch_printf(ch, "&GIt costs %s %ld credits to ready this ship for launch.\n", ch->pcdata->clan->name, price );
  }
  else if ( str_cmp( ship->owner , "Public" ) )
  {
    if ( ch->gold < price )
    {
      ch_printf(ch, "&RYou don't have enough funds to prepare this ship for launch.\n");
      return;
    }

    ch->gold -= price;
    ch_printf(ch, "&GYou pay %ld credits to ready the ship for launch.\n", price );
  }

  ship->energy = ship->mod->maxenergy;
  ship->shield = 0;
  ship->autorecharge = FALSE;
  ship->autotrack = FALSE;
  ship->autospeed = FALSE;
  ship->hull = ship->mod->maxhull;

  ship->missilestate = MISSILE_READY;
  ship->statet0 = LASER_READY;
  ship->shipstate = SHIP_LANDED;

  return;

 }

void do_openbay( CHAR_DATA *ch, char *argument )
{ 
  SHIP_DATA *ship;
  char buf[MAX_STRING_LENGTH];

   if ( ship_from_pilotseat(ch->game, ch->in_room->vnum) == NULL
   && ship_from_hanger(ch->game, ch->in_room->vnum) == NULL )
   {
        send_to_char("&RYou aren't in the pilots chair or hanger of a ship!\n",ch);
        return;
   }

   if ( ship_from_pilotseat(ch->game, ch->in_room->vnum) )
      ship = ship_from_pilotseat(ch->game, ch->in_room->vnum);
   else
      ship = ship_from_hanger(ch->game, ch->in_room->vnum);

   if ( ship->hanger == 0 )
   {
      send_to_char("&RThis ship has no hanger!\n",ch);
      return;
   }

   if (ship->bayopen == TRUE)
   {
      send_to_char("Bay doors are already open!",ch);
      return;
   }

   act( AT_PLAIN, "$n flips a switch on the control panel.", ch,
         NULL, argument , TO_ROOM );
      ship->bayopen = TRUE;

      echo_to_cockpit( AT_YELLOW , ship, "Bay Doors Open");
      send_to_char("You open the bay doors", ch);
      SPRINTF( buf ,"%s's bay doors open." , ship->name );
      echo_to_system( AT_YELLOW, ship, buf , NULL );

   }

void do_closebay( CHAR_DATA *ch, char *argument )
{
  SHIP_DATA *ship;
  char buf[MAX_STRING_LENGTH];
      if ( ship_from_pilotseat(ch->game, ch->in_room->vnum) == NULL
   && ship_from_hanger(ch->game, ch->in_room->vnum) == NULL )
   {
        send_to_char("&RYou aren't in the pilots chair or hanger of a ship!\n",ch);
        return;
   }

   if ( ship_from_pilotseat(ch->game, ch->in_room->vnum) )
      ship = ship_from_pilotseat(ch->game, ch->in_room->vnum);
   else
      ship = ship_from_hanger(ch->game, ch->in_room->vnum);

   if ( ship->hanger == 0 )
   {
      send_to_char("&RThis ship has no hanger!\n",ch);
      return;
   }

   if (ship->bayopen == FALSE)
   {
      send_to_char("Bay doors are already closed!", ch);
      return;
   }

   act( AT_PLAIN, "$n flips a switch on the control panel.", ch,
         NULL, argument , TO_ROOM );
      ship->bayopen = FALSE;

      echo_to_cockpit( AT_YELLOW , ship, "Bay Doors close");
      send_to_char("You close the bay doors.", ch);
      SPRINTF( buf ,"%s's bay doors close." , ship->name );
      echo_to_system( AT_YELLOW, ship, buf , NULL );

   }

#if 0
void do_tractorbeam( CHAR_DATA *ch, char *argument )
{

    char arg[MAX_INPUT_LENGTH];
    int chance;
    SHIP_DATA *ship;
    SHIP_DATA *target;
	char buf[MAX_STRING_LENGTH];

    SPRINTF( arg, argument );

	if ( (ship = ship_from_cockpit(ch->in_room->vnum)) == NULL )
	{
		send_to_char("&RYou must be in the cockpit of a ship to do that!\n",ch);
		return;
	}

	if ( ship->shipclass > SHIP_PLATFORM )
	{
		send_to_char("&RThis isn't a spacecraft!\n",ch);
		return;
	}


	if ( !check_pilot( ch , ship ) )
	{
		send_to_char("This isn't your ship!\n" , ch );
		return;
	}

	if ( ship->mod->tractorbeam == 0 )
	{
		send_to_char("You might want to install a tractorbeam!\n" , ch );
		return;
	}

	if ( ship->hanger == 0 )
	{
		send_to_char("No hanger available.\n",ch);
		return;
	}

	if ( !ship->bayopen )
	{
		send_to_char("Your hanger is closed.\n",ch);
		return;
	}


	if ( (ship = ship_from_pilotseat(ch->game, ch->in_room->vnum)) == NULL )
	{
		send_to_char("&RYou need to be in the pilot seat!\n",ch);
		return;
	}


	if (ship->shipstate == SHIP_DISABLED)
	{
		send_to_char("&RThe ships drive is disabled. No power available.\n",ch);
		return;
	}

	if (ship->shipstate == SHIP_HYPERSPACE)
	{

		send_to_char("&RYou can only do that in realspace!\n",ch);
		return;
	}

	if (ship->shipstate != SHIP_READY)
	{
		send_to_char("&RPlease wait until the ship has finished its current manouver.\n",ch);
		return;
	}




	if ( argument[0] == '\0' )
	{
		send_to_char("&RCapture what?\n",ch);
		return;
	}

	target = get_ship_here( argument , ship );

	if ( target == NULL )
	{
		send_to_char("&RI don't see that here.\n",ch);
		return;
	}

   if ( target->docked != NULL )
   {
      send_to_char("&RThat ship is docked!\n",ch);
      return;
   }
	if ( target == ship )
	{
		send_to_char("&RYou can't capture yourself!\n",ch);
		return;
	}

	if (  (abs(target->vx - ship->vx) >= (100+ship->mod->tractorbeam*2)) ||
	   (abs(target->vy - ship->vy) >= (100+ship->mod->tractorbeam*2)) ||
	   (abs(target->vz - ship->vz) >= (100+ship->mod->tractorbeam*2) ) )
	{
		send_to_char("&R That ship is too far away! You'll have to fly a litlle closer.\n",ch);
		return;
	}

	if (ship->shipclass != SHIP_DEBRIS && ship->shipclass <= target->shipclass)
	{
		send_to_char("&RThat ship is too big for your hanger.\n",ch);
		return;
	}

	if  ( target->shipclass == SHIP_PLATFORM )
	{
		send_to_char( "&RYou can't capture platforms.\n" , ch );
		return;
	}

	if ( target->shipclass == CAPITAL_SHIP)
	{
		send_to_char("&RYou can't capture capital ships.\n",ch);
		return;
	}


	if ( ship->energy < (25 + 25*target->shipclass) )
	{
		send_to_char("&RTheres not enough fuel!\n",ch);
		return;
	}




	chance = IS_NPC(ch) ? ch->top_level
	: (int)  (ch->pcdata->learned[gsn_tractorbeams]);

	/* This is just a first guess chance modifier, feel free to change if needed */

	chance = chance * ( ship->mod->tractorbeam / (target->currspeed+1 ) );

	if ( number_percent( ) < chance )
	{
		set_char_color( AT_GREEN, ch );
		send_to_char( "Capture sequence initiated.\n", ch);
		act( AT_PLAIN, "$n begins the capture sequence.", ch,
			NULL, argument , TO_ROOM );
		echo_to_ship( AT_YELLOW , ship , "ALERT: Ship is being captured, all hands to docking bay." );
    	echo_to_ship( AT_YELLOW , target , "The ship shudders as a tractorbeam locks on." );
		SPRINTF( buf , "You are being captured by %s." , ship->name);
		echo_to_cockpit( AT_BLOOD , target , buf );

		if ( (target->autopilot || target->type == MOB_SHIP) && !target->target0)
			target->target0 = ship;


		target->dest = STRALLOC(ship->name);
		target->shipstate = SHIP_LAND;
		target->currspeed = 0;
      target->autopilot = FALSE;

		learn_from_success( ch, gsn_tractorbeams );
		return;

	}
	send_to_char("You fail to work the controls properly.\n",ch);
   	echo_to_ship( AT_YELLOW , target , "The ship shudders and then stops as a tractorbeam attemps to lock on and fails." );
	SPRINTF( buf , "The %s attempted to capture your ship!" , ship->name);
	echo_to_cockpit( AT_BLOOD , target , buf );
	if ( (target->autopilot || target->type == MOB_SHIP) && !target->target0)
		target->target0 = ship;


	learn_from_failure( ch, gsn_tractorbeams );

   	return;
}
#endif

void do_tractorbeam(CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    int chance, origchance, distance;
    SHIP_DATA *ship;
    SHIP_DATA *target;
    char buf[MAX_STRING_LENGTH];

    SPRINTF( arg, "%s", argument );

    switch( ch->substate )
    {
    	default:
    	        if (  (ship = ship_from_coseat(ch->game, ch->in_room->vnum))  == NULL )
    	        {
    	            send_to_char("&RYou must be in the copilot's seat of a ship to do that!\n",ch);
    	            return;
    	        }

                if ( ship->shipclass > SHIP_PLATFORM )
    	        {
    	            send_to_char("&RThis isn't a spacecraft!\n",ch);
    	            return;
    	        }
                if ( ship->mod->tractorbeam == 0 )
    	        {
    	            send_to_char("&RThis craft does not have a tractorbeam!\n",ch);
    	            return;
    	        }

		

                if (ship->shipstate == SHIP_HYPERSPACE || !ship->spaceobject )
                {
                  send_to_char("&RYou can only do that in realspace!\n",ch);
                  return;
                }
		
                if (ship->docking != SHIP_READY )
                {
                    send_to_char("&RThe ship structure can not tolerate pressure from both tractorbeam and docking port.\n",ch);
                    return;
                }
                if (ship->shipstate == SHIP_TRACTORED)
                {
                    send_to_char("&RYou can not move in a tractorbeam!\n",ch);
                    return;
                }

                if ( autofly(ship) )
    	        {
    	            send_to_char("&RYou'll have to turn off the ships autopilot first....\n",ch);
    	            return;
    	        }

                if (arg[0] == '\0')
    	        {
    	            send_to_char("&RYou need to specify a target!\n",ch);
    	            return;
    	        }

                if ( !str_cmp( arg, "none") )
    	        {
    	            send_to_char("&GTractorbeam set to no target.\n",ch);
		    if( ship->tractored && ship->tractored->tractoredby == ship )
		    {
		      ship->tractored->tractoredby = NULL;
		      if( ship->tractored->location )
		        ship->tractored->shipstate = SHIP_LANDED;
		      else if ( ship->tractored->shipstate != SHIP_DOCKED && ship->tractored->shipstate != SHIP_DISABLED )
		        ship->tractored->shipstate = SHIP_READY;
		      
		    }
		    ship->tractored = NULL;
    	            return;
    	        }

		if( ship->tractored )
    	        {
    	            send_to_char("&RReleasing previous target.\n",ch);
                  ship->tractored->tractoredby = NULL;
                  if( ship->tractored->location )
                    ship->tractored->shipstate = SHIP_LANDED;
                  else if ( ship->tractored->shipstate != SHIP_DOCKED && ship->tractored->shipstate != SHIP_DISABLED )
                    ship->tractored->shipstate = SHIP_READY;
    	        }
		  

                target = get_ship_here( ship->game, arg, ship );


                if (  target == NULL )
                {
                    send_to_char("&RThat ship isn't here!\n",ch);
                    return;
                }

                if (  target == ship )
                {
                    send_to_char("&RYou can't tractor your own ship!\n",ch);
                    return;
                }

                if ( !str_cmp(ship->owner, "Trainer") && str_cmp(target->owner, "Trainer") )
                {
                    send_to_char("&RTrainers can only target other trainers!!\n",ch);
                    return;
                }
                if ( str_cmp(ship->owner, "Trainer") && !str_cmp(target->owner, "Trainer") )
                {
                    send_to_char("&ROnly trainers can target other trainers!!\n",ch);
                    return;
                }

		if ( ship->energy < (25 + 25*target->shipclass) )
		{
			send_to_char("&RTheres not enough fuel!\n",ch);
			return;
		}
               if( ship->shipclass <= SHIP_PLATFORM)
               {
                if ( abs( (int) ( ship->vx-target->vx )) > 100+ship->mod->tractorbeam ||
                     abs( (int) ( ship->vy-target->vy )) > 100+ship->mod->tractorbeam ||
                     abs( (int) ( ship->vz-target->vz )) > 100+ship->mod->tractorbeam )
                {
                    send_to_char("&RThat ship is too far away to tractor.\n",ch);
                    return;
                }
               }

                chance = IS_NPC(ch) ? ch->top_level
	                 : (int)  (ch->pcdata->learned[gsn_tractorbeams]) ;

                if ( number_percent( ) < chance )
    		{
    		   send_to_char( "&GTracking target.\n", ch);
    		   act( AT_PLAIN, "$n makes some adjustments on the targeting computer.", ch,
		        NULL, argument , TO_ROOM );
    		   add_timer ( ch , TIMER_DO_FUN , 1 , do_tractorbeam , 1 );
    		   ch->dest_buf = str_dup(arg);
    		   return;
	        }
	        send_to_char("&RYou fail to work the controls properly.\n",ch);
	        learn_from_failure( ch, gsn_tractorbeams );
    	   	return;

    	case 1:
    		if ( !ch->dest_buf )
    		   return;
    		SPRINTF(arg, "%s", (const char * ) ch->dest_buf);
    		STR_DISPOSE( ch->dest_buf);
    		break;

    	case SUB_TIMER_DO_ABORT:
    		STR_DISPOSE( ch->dest_buf );
    		ch->substate = SUB_NONE;
    		if ( (ship = ship_from_cockpit(ch->game, ch->in_room->vnum)) == NULL )
    		      return;
    	        send_to_char("&RYour concentration is broken. You fail to lock onto your target.\n", ch);
    		return;
    }

    ch->substate = SUB_NONE;

    if ( (ship = ship_from_coseat(ch->game, ch->in_room->vnum)) == NULL )
    {
       return;
    }
    target = get_ship_here( ship->game, arg, ship );

    if (  target == NULL || target == ship)
    {
           send_to_char("&RThe ship has left the starsytem. Targeting aborted.\n",ch);
           return;
    }
                chance = IS_NPC(ch) ? ch->top_level
	                 : (int)  (ch->pcdata->learned[gsn_tractorbeams]) ;
                origchance = chance;
             distance = abs( (int) ( target->vx - ship->vx )) 
               + abs( (int) ( target->vy - ship->vy )) 
               + abs( (int) ( target->vz - ship->vz ));
	     distance /= 3;
	     chance += target->shipclass - ship->shipclass;
	     chance += ship->currspeed - target->currspeed;
	     chance += ship->mod->manuever - target->mod->manuever;
	     chance -= distance/(10*(target->shipclass+1));
	     chance -= origchance;
	     chance /= 2;
	     chance += origchance;
             chance = URANGE( 1 , chance , 99 );

                if ( number_percent( ) >= chance )
    		{
	        send_to_char("&RYou fail to work the controls properly.\n",ch);
	        learn_from_failure( ch, gsn_tractorbeams );
    	   	return;
	        }

    ship->tractored = target;
    target->tractoredby = ship;
    target->shipstate = SHIP_TRACTORED;
    ship->energy -= 25 + 25*target->shipclass;


     if ( target->shipclass <= ship->shipclass )
     {
	    target->currspeed = ship->mod->tractorbeam/2;
	    target->hx = ship->vx - target->vx;
	    target->hy = ship->vy - target->vy;
	    target->hz = ship->vz - target->vz;
     }
     if ( target->shipclass > ship->shipclass )
     {

	    ship->currspeed = ship->mod->tractorbeam/2;
	    ship->hx = target->vx - ship->vx;
	    ship->hy = target->vy - ship->vy;
	    ship->hz = target->vz - ship->vz;
     }
          
    send_to_char( "&GTarget Locked.\n", ch);
    SPRINTF( buf , "You have been locked in a tractor beam by %s." , ship->name);  
    echo_to_cockpit( AT_BLOOD , target , buf );

    sound_to_room( ch->in_room , "!!SOUND(targetlock)" );
    learn_from_success( ch, gsn_tractorbeams );
    	
    if ( autofly(target) && !target->target0 && str_cmp( target->owner, ship->owner ) )
    {
       SPRINTF( buf , "You are being targetted by %s." , target->name);  
       echo_to_cockpit( AT_BLOOD , ship , buf );
       target->target0 = ship;
    }
}

void do_adjusttractorbeam(CHAR_DATA *ch, char *argument )
{
  char buf[MAX_INPUT_LENGTH];
  char arg[MAX_INPUT_LENGTH];
  SHIP_DATA *ship, *eShip;

  SPRINTF( arg, "%s", argument );
  
    
  if (  (ship = ship_from_coseat(ch->game, ch->in_room->vnum))  == NULL )
  {
    send_to_char("&RYou must be in the copilot's seat of a ship to do that!\n",ch);
    return;
  }

  if ( !ship->tractored || ship->tractored->tractoredby != ship )
  {
    if ( ship->tractored && ship->tractored->tractoredby != ship )
      ship->tractored = NULL;
    send_to_char("&RYour tractor beam is not trained on a ship.\n",ch);
    return;
  }
    

  if (arg[0] == '\0')
  {
    SPRINTF( buf, "&RCurrent tractor beam settings: ");
    if( ship->statettractor == SHIP_DISABLED )
      STRAPP( buf, "Disabled.\n" );
    if( ship->tractored == NULL )
      STRAPP( buf, "Deactivated.\n" );
    if( ship->tractored && ship->tractored->shipstate == SHIP_TRACTORED )
      STRAPP( buf, "Pulling Target.\n" );
    if( ship->tractored && ship->tractored->shipstate >= SHIP_DOCKED )
      STRAPP( buf, "Docking Port Approach.\n" );
    if( ship->tractored && ( ship->tractored->shipstate == SHIP_LAND_2 || ship->tractored->shipstate == SHIP_LAND ) )
      STRAPP( buf, "Hanger Approach.\n" );
    ch_printf(ch, "&RCurrent tractor beam settings: %s\n", buf);
    return;
  }

  eShip = ship->tractored;

    act( AT_PLAIN, "$n flips a switch on the control panell.", ch,
         NULL, argument , TO_ROOM );

  if( str_cmp( arg, "undock" ) && eShip->docked && eShip->docked != ship)
  {
    echo_to_cockpit( AT_YELLOW, ship, "Tractor Beam set on docked ship. Undock it first.\n" );
    return;
  }
  
  if( eShip->shipclass >= ship->shipclass && eShip->shipclass != SHIP_DEBRIS )
  {
    echo_to_cockpit( AT_YELLOW, ship, "Tractor Beam set on ship of a greater or equal mass as our own. It will not move.\n" );
    return;
  }

  if ( !eShip->spaceobject )
  {
    echo_to_cockpit( AT_YELLOW, ship, "Target is on the ground. There is no need to adjust the tractor beam.\n" );
    return;
  }
  
  
  if ( !str_cmp( arg, "pull") || !str_cmp( arg, "none" ) )
  {
    echo_to_cockpit( AT_YELLOW, ship, "Tractor Beam set to pull target.\n" );
    eShip->shipstate = SHIP_TRACTORED;
    eShip->docked = NULL;
    eShip->docking = SHIP_READY;
    STRFREE(eShip->dest);
    return;
  }
  if ( !str_cmp( arg, "abort" ) )
  {
    echo_to_cockpit( AT_YELLOW, ship, "Manuever aborted. Tractor beam returned to default setting.\n" );
    eShip->shipstate = SHIP_TRACTORED;
    eShip->docked = NULL;
    eShip->docking = SHIP_READY;
    STRFREE(eShip->dest);
    return;
  }
  
  if ( !str_cmp( arg, "dock") )
  {
    if ( abs( (int) ( ship->vx-eShip->vx )) > 100 ||
         abs( (int) ( ship->vy-eShip->vy )) > 100 ||
         abs( (int) ( ship->vz-eShip->vz )) > 100 )
    {
      send_to_char("&RYou aren't close enough to dock target.\n",ch);
      return;
    }
    if ( !candock( eShip ) || !candock( ship ) )
    {
      send_to_char("&RYou have no empty docking port.\n",ch);
      return;
    }
    
    echo_to_cockpit( AT_YELLOW, ship, "Tractor Beam set to dock target.\n" );
    eShip->docking = SHIP_DOCK;
    eShip->docked = ship;
    return;
  }
  if ( !str_cmp( arg, "land") )
  {
    if ( abs( (int) ( ship->vx-eShip->vx )) > 100 ||
         abs( (int) ( ship->vy-eShip->vy )) > 100 ||
         abs( (int) ( ship->vz-eShip->vz )) > 100 )
    {
      send_to_char("&RYou aren't close enough to the target to pull it into your hanger.\n",ch);
      return;
    }
    if ( !ship->hanger )
    {
      send_to_char("&RYou have no hanger!\n",ch);
      return;
    }
    if( !ship->bayopen )
    {
      send_to_char("&RThe bay is not open.\n",ch);
      return;
    }
    if( ship->shipclass < eShip->shipclass || eShip->shipclass == SHIP_PLATFORM || eShip->shipclass == CAPITAL_SHIP )
    {
      send_to_char("&RThat ship can not land in your bay.\n",ch);
      return;
    }
    
    
    echo_to_cockpit( AT_YELLOW, ship, "Tractor Beam set to land target.\n" );
    eShip->shipstate = SHIP_LAND;
    eShip->dest = STRALLOC(ship->name);
    return;
  }

  if ( !str_cmp( arg, "undock" ) )
  {
    if ( abs( (int) ( ship->vx-eShip->vx )) > 100 ||
         abs( (int) ( ship->vy-eShip->vy )) > 100 ||
         abs( (int) ( ship->vz-eShip->vz )) > 100 )
    {
      send_to_char("&RYou aren't close enough to the target to pull it off its position.\n",ch);
      return;
    }
    if ( !eShip->docked )  
    {
      send_to_char("&RYour target is not docked.\n",ch);
      return;
    }
    echo_to_cockpit( AT_YELLOW, ship, "Tractor beam set to undock target.\n" );
    eShip->shipstate = SHIP_TRACTORED;
    eShip->docked->statettractor = SHIP_DISABLED;
    eShip->statettractor = SHIP_DISABLED;
    echo_to_cockpit( AT_RED, eShip, "As a ship is torn from your docking bay, the clamps are damaged!." );
    echo_to_cockpit( AT_RED, ship, "As your ship is torn from the docking bay, the clamps are damaged!." );
    eShip->docked = NULL;
    eShip->docking = SHIP_READY;
    return;
  }    
}

void do_undock(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    
    int chance = 0;
    SHIP_DATA *ship;  
    SHIP_DATA *eShip = NULL;
    
    SPRINTF( arg, "%s", argument );
        
        if (  (ship = ship_from_cockpit(ch->game, ch->in_room->vnum))  == NULL )
        {
            send_to_char("&RYou must be in the cockpit of a ship to do that!\n",ch);
            return;
        }

        if ( ship->shipclass > SHIP_PLATFORM )
                {
                    send_to_char("&RThis isn't a spacecraft!\n",ch);
                    return;
                }
    
        if (  (ship = ship_from_pilotseat(ch->game, ch->in_room->vnum))  == NULL )
        {
            send_to_char("&RYou aren't in the pilots seat.\n",ch);
            return;
        }

        if ( (ship->autopilot || ship->type == MOB_SHIP)  )
                {
                    send_to_char("&RYou'll have to turn off the ships autopilot first.\n",ch);
                    return;
               }

                if  ( ship->shipclass == SHIP_PLATFORM )
                {
                   send_to_char( "&RPlatforms can't move!\n" , ch );
                   return;
                }
                if (ship->shipstate == SHIP_HYPERSPACE)
               {
                  send_to_char("&RYou can only do that in realspace!\n",ch);
                  return;
               }
                if ( ship->docked && ship->tractoredby &&
                    ship->docked != ship->tractoredby )
                {
                    send_to_char("&RYou can not do that in a tractor beam!\n",ch);
                    return;
                }
                 
                if (ship->docked == NULL && ship->docking == SHIP_READY)
                {
                    send_to_char("&RYou aren't docked!\n",ch);
                    return;
                }
		eShip = ship->docked;

	if ( ship->shipclass == FIGHTER_SHIP )
             chance = IS_NPC(ch) ? ch->top_level
             : (int)  (ch->pcdata->learned[gsn_starfighters]) ;
        if ( ship->shipclass == MIDSIZE_SHIP )
             chance = IS_NPC(ch) ? ch->top_level
                 : (int)  (ch->pcdata->learned[gsn_midships]) ;
        if ( ship->shipclass == CAPITAL_SHIP )
              chance = IS_NPC(ch) ? ch->top_level
                 : (int) (ch->pcdata->learned[gsn_capitalships]);
        if ( number_percent( ) > chance )
        {
            send_to_char("&RYou can't figure out which lever to use.\n",ch);
            if ( ship->shipclass == FIGHTER_SHIP )
                {
                learn_from_failure( ch, gsn_starfighters );
                learn_from_failure( ch, gsn_shipdocking);
                }
            if ( ship->shipclass == MIDSIZE_SHIP )
                {
                learn_from_failure( ch, gsn_midships );
                learn_from_failure( ch, gsn_shipdocking);
                }
            if ( ship->shipclass == CAPITAL_SHIP )
                {
                learn_from_failure( ch, gsn_capitalships );
                learn_from_failure( ch, gsn_shipdocking);
                }
           return;
        }           
	if( ship->docking == SHIP_DOCKED )
                echo_to_ship( AT_YELLOW , ship , "The ship unlocks the clamps and begins to drift away.");
	else
                echo_to_ship( AT_YELLOW , ship , "You abort the docking manuever.");

	      if ( ship->location )
	        ship->shipstate = SHIP_LANDED;
	      else                
      		ship->shipstate = SHIP_READY;
		    ship->docking = SHIP_READY;
        ship->docked = NULL;

	      if( eShip )
	      {             
                echo_to_ship( AT_YELLOW , eShip , "Ship undocking. Clamps released.");
//              eShip->docked = NULL;  // No need... ship docking is only one way.
//	      if ( eShip->location )
//	        eShip->shipstate = SHIP_LANDED;
//	      else                
//		eShip->shipstate = SHIP_READY;
 	      }


            if ( ship->shipclass == FIGHTER_SHIP )
                {
                learn_from_success( ch, gsn_starfighters );
                learn_from_success( ch, gsn_shipdocking);
                }
            if ( ship->shipclass == MIDSIZE_SHIP )
                {
                learn_from_success( ch, gsn_midships );  
                learn_from_success( ch, gsn_shipdocking);
                }
            if ( ship->shipclass == CAPITAL_SHIP )
                {
                learn_from_success( ch, gsn_capitalships );
                learn_from_success( ch, gsn_shipdocking);
                }
                 

}
                
bool candock( SHIP_DATA *ship )
{
  int count = 0;
  SHIP_DATA *dship;
  int ports;
  
  if ( !ship )
    return FALSE;
  
  if ( ship->docked ) 
    count++;
    
  for( dship = first_ship; dship; dship = dship->next )
    if( dship->docked && dship->docked == ship )
      count++;
      
  if ( ship->dockingports && count >= ship->dockingports )
    return FALSE;
  else if (!(ship->dockingports))
  {  
    if ( ship->shipclass < SHIP_PLATFORM )
      ports = ship->shipclass + 1;

    if ( ship->shipclass != SHIP_PLATFORM && count >= ports )
      return FALSE;
  }
  return TRUE;
}

void do_dock(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];

    int chance = 0;
    SHIP_DATA *ship;
    SHIP_DATA *eShip = NULL;

    SPRINTF( arg, "%s", argument );

        if (  (ship = ship_from_cockpit(ch->game, ch->in_room->vnum))  == NULL )
        {
            send_to_char("&RYou must be in the cockpit of a ship to do that!\n",ch);
            return;
        }

        if ( ship->shipclass > SHIP_PLATFORM )
                {
                    send_to_char("&RThis isn't a spacecraft!\n",ch);
                    return;
                }


    	        if (! ship->spaceobject )
    	        {
    	            send_to_char("&RYou can't do that until you've finished launching!\n",ch);
    	            return;
    	        }

        if (  (ship = ship_from_pilotseat(ch->game, ch->in_room->vnum))  == NULL )
        {
            send_to_char("&RYou aren't in the pilots seat.\n",ch);
            return;
        }

        if ( (ship->autopilot || ship->type == MOB_SHIP)  )
                {
                    send_to_char("&RYou'll have to turn off the ships autopilot first.\n",ch);
                    return;
               }

                if  ( ship->shipclass == SHIP_PLATFORM )
                {
                   send_to_char( "&RPlatforms can't move!\n" , ch );
                   return;
                }
                if (ship->shipstate == SHIP_HYPERSPACE)
               {
                  send_to_char("&RYou can only do that in realspace!\n",ch);
                  return;
               }

                if (ship->shipstate == SHIP_DISABLED)
                {
                    send_to_char("&RThe ships drive is disabled. Unable to manuever.\n",ch);
                    return;
                }
                if (ship->statetdocking == SHIP_DISABLED)
                {
                    send_to_char("&RYour docking port is damaged. Get it repaired!\n",ch);
                    return;
                }

    	        if (ship->docking == SHIP_DOCKED)
    	        {
    	            send_to_char("&RTry undocking first!\n",ch);
    	            return;
    	        }
    	        if (!candock(ship))
    	        {
    	            send_to_char("&RTry undocking first!\n",ch);
    	            return;
    	        }
                if (ship->shipstate == SHIP_LANDED)
                {
                    send_to_char("&RYou are already docked!\n",ch);
                    return;
                }
                if (ship->shipstate == SHIP_TRACTORED && ship->tractoredby && ship->tractoredby->shipclass >= ship->shipclass )
                {
                    send_to_char("&RYou can not move in a tractorbeam!\n",ch);
                    return;
                }
                if (ship->tractored )
                {
                    send_to_char("&RThe ship structure can not tolerate stresses from both tractorbeam and docking port simultaneously.\n",ch);
                    return;
                }
                if (ship->shipstate != SHIP_READY)
                {
                    send_to_char("&RPlease wait until the ship has finished its current manouver.\n",ch);
                    return;
                }

        if ( ship->currspeed < 1 )
        {
              send_to_char("&RYou need to speed up a little first!\n",ch);
              return;
        }
        if ( ship->currspeed > 120 )
        {
              send_to_char("&RYou need to slow down first!\n",ch);
              return;
        }

               if (arg[0] == '\0')
    	        {
    	            send_to_char("&RDock where?\n",ch);
    	            return;
    	        }

      eShip = get_ship_here( ship->game, arg, ship );

                if (  eShip == NULL )
                {
                    send_to_char("&RThat ship isn't here!\n",ch);
                    return;
                }
                if (  eShip == ship )
                {
                    send_to_char("&RYou can't dock with your own ship!\n",ch);
                    return;
                }
	if( ship->shipclass > eShip->shipclass )
                {
                    send_to_char("&RYou can not dock with a ship smaller than yours. Have them dock to you.\n",ch);
                    return;
                }
	

	if (!candock(eShip))
    	{
    	  send_to_char("&RYou can not seem to find an open docking port.\n",ch);
	  return;
	}


       if ( eShip->currspeed >0 )
        {
              send_to_char("&RThey need to be at a dead halt for the docking maneuver to begin.\n",ch);
              return;
        }

       if ( autofly(eShip)  )
                {
                    send_to_char("&RThe other ship needs to turn their autopilot off.\n",ch);
                    return;
               }

                if ( abs( (int) ( ship->vx-eShip->vx )) > 100 ||
                     abs( (int) ( ship->vy-eShip->vy )) > 100 ||
                     abs( (int) ( ship->vz-eShip->vz )) > 100 )
                {
                    send_to_char("&RYou aren't close enough to dock.  Get a little closer first then try again.\n",ch);
                    return;
                }



        if ( ship->shipclass == FIGHTER_SHIP )
             chance = IS_NPC(ch) ? ch->top_level
             : (int)  (ch->pcdata->learned[gsn_starfighters]) ;
        if ( ship->shipclass == MIDSIZE_SHIP )
             chance = IS_NPC(ch) ? ch->top_level
                 : (int)  (ch->pcdata->learned[gsn_midships]) ;
        if ( ship->shipclass == CAPITAL_SHIP )
              chance = IS_NPC(ch) ? ch->top_level
                 : (int) (ch->pcdata->learned[gsn_capitalships]);
        if ( number_percent( ) > chance )
        {
            send_to_char("&RYou can't figure out which lever to use.\n",ch);
            if ( ship->shipclass == FIGHTER_SHIP )
                {
                learn_from_failure( ch, gsn_starfighters );
                learn_from_failure( ch, gsn_shipdocking);
                }
            if ( ship->shipclass == MIDSIZE_SHIP )
                {
                learn_from_failure( ch, gsn_midships );
                learn_from_failure( ch, gsn_shipdocking);
                }
            if ( ship->shipclass == CAPITAL_SHIP )
                {
                learn_from_failure( ch, gsn_capitalships );
                learn_from_failure( ch, gsn_shipdocking);
                }
           return;
        }
                echo_to_ship( AT_YELLOW , ship , "The ship slowly begins its docking maneveurs.");
                echo_to_ship( AT_YELLOW , eShip , "The ship slowly begins its docking maneveurs.");
		ship->docked = eShip;
		ship->docking= SHIP_DOCK;
		ship->ch = ch;
  		return;
/*
    	case 1:
    		if ( !ch->dest_buf )
    		   return;
    		SPRINTF(arg, ch->dest_buf);
    		STR_DISPOSE( ch->dest_buf);
    		break;

    	case SUB_TIMER_DO_ABORT:
    		STR_DISPOSE( ch->dest_buf );
    		ch->substate = SUB_NONE;
		ship->docked = NULL;
    	        send_to_char("&RDocking maneuver aborted.\n", ch);
    		return;
    }

    ch->substate = SUB_NONE;
*/
}

void dockship( CHAR_DATA *ch, SHIP_DATA *ship )
{

	if ( ship->statetdocking == SHIP_DISABLED )
	{
                echo_to_ship( AT_YELLOW , ship , "Maneuver Aborted. Docking clamps damaged.");
                echo_to_ship( AT_YELLOW , ship->docked, "The ship aborted the docking manuever.");
                ship->docking = SHIP_READY;
                ship->docked = NULL;
                return;
        }
	if ( ship->docked->statetdocking == SHIP_DISABLED )
	{
                echo_to_ship( AT_YELLOW , ship->docked , "Maneuver Aborted. Docking clamps damaged.");
                echo_to_ship( AT_YELLOW , ship, "The ship aborted the docking manuever.");
                ship->docking = SHIP_READY;
                ship->docked = NULL;
                return;
        }

                echo_to_ship( AT_YELLOW , ship , "The ship finishing its docking manuevers.");
                echo_to_ship( AT_YELLOW , ship->docked, "The ship finishes its docking manuevers.");

                ship->docking = SHIP_DOCKED;
                ship->currspeed = 0;
		ship->vx = ship->docked->vx;
		ship->vy = ship->docked->vy;
		ship->vz = ship->docked->vz;
	  if( ch )
	  {
            if ( ship->shipclass == FIGHTER_SHIP )
                {
                learn_from_success( ch, gsn_starfighters );
                learn_from_success( ch, gsn_shipdocking);
                }
            if ( ship->shipclass == MIDSIZE_SHIP )
                {
                learn_from_success( ch, gsn_midships );
                learn_from_success( ch, gsn_shipdocking);
                }
            if ( ship->shipclass == CAPITAL_SHIP )
                {
                learn_from_success( ch, gsn_capitalships );
                learn_from_success( ch, gsn_shipdocking);
        	}
          }
}

void do_request(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int chance = 0;
    SHIP_DATA *ship;
    SHIP_DATA *eShip = NULL;

    SPRINTF( arg, "%s", argument );

    if ( (ship = ship_from_cockpit(ch->game, ch->in_room->vnum)) == NULL )
    {
        send_to_char("&RYou must be in the cockpit of a ship to do that!\n",ch);
        return;
    }

    if ( ship->shipclass > SHIP_PLATFORM )
    {
        send_to_char("&RThis isn't a spacecraft!",ch);
        return;
    }

    if ( !ship->spaceobject )
    {
        send_to_char("&RYou can't do that until you've finished launching!\n",ch);
        return;
    }

    if (ship->shipstate == SHIP_HYPERSPACE )
    {
        send_to_char("&RYou can only do that in realspace!\n",ch);
        return;
    }

    if ( arg[0] == '\0' )
    {
        send_to_char("&RRequest the opening of the baydoors of what ship?\n",ch);
        return;
    }

    eShip = get_ship_here(ship->game, arg,ship);

    if ( eShip == NULL )
    {
        send_to_char("&RThat ship isn't here!\n",ch);
        return;
    }

    if ( eShip == ship )
    {
        send_to_char("&RIf you have bay doors, why not open them yourself?\n",ch);
        return;
    }

    if ( eShip->hanger == 0 )
    {
        send_to_char("&RThat ship has no hanger!",ch);
        return;
    }

    if ( !autofly(eShip) )
    {
        send_to_char("&RThe other ship needs to have its autopilot turned on.\n",ch);
        return;
    }
    if ( abs( (int) ( eShip->vx - ship->vx )) > 100*((ship->mod->comm)+(eShip->mod->comm)+20) ||
    abs( (int) ( eShip->vy - ship->vy )) > 100*((ship->mod->comm)+(eShip->mod->comm)+20) ||
    abs( (int) ( eShip->vz - ship->vz )) > 100*((ship->mod->comm)+(eShip->mod->comm)+20) )

    {
      send_to_char("&RThat ship is out of the range of your comm system.\n&w", ch);
      return;
    }

    if ( abs( (int) ( eShip->vx - ship->vx )) > 100*(ship->mod->sensor+10)*((eShip->shipclass)+1) ||
    abs( (int) ( eShip->vy - ship->vy )) > 100*(ship->mod->sensor+10)*((eShip->shipclass)+1) ||
    abs( (int) ( eShip->vz - ship->vz )) > 100*(ship->mod->sensor+10)*((eShip->shipclass)+1) )
    {
      send_to_char("&RThat ship is too far away to remotely open bay doors.\n",ch);
      return;
    }


    chance = IS_NPC(ch) ? ch->top_level : (int) (ch->pcdata->learned[gsn_fake_signal]);
    if ( (eShip->shipclass == SHIP_PLATFORM ? 1 : (number_percent( ) >= chance)) && !check_pilot(ch,eShip) )
    {
        send_to_char("&RHey! That's not your ship!",ch);
        return;
    }

    if ( eShip->bayopen == TRUE )
    {
        send_to_char("&RThat ship's bay doors are already open!\n",ch);
        return;
    }
    if ( chance && !check_pilot(ch, eShip) )
      learn_from_success(ch, gsn_fake_signal);
      
    send_to_char("&RYou open the bay doors of the remote ship.",ch);
    act(AT_PLAIN,"$n flips a switch on the control panel.",ch,NULL,argument,TO_ROOM);
    eShip->bayopen = TRUE;
    SPRINTF( buf ,"%s's bay doors open." , eShip->name );
    echo_to_system( AT_YELLOW, ship, buf , NULL );
}

void do_shiptrack( CHAR_DATA *ch, char *argument)
{
  SHIP_DATA *ship;
  SPACE_DATA *spaceobject;
  std::string arg;
  std::string arg1;
  std::string arg2;
  std::string arg3;  
  std::string buf;
  std::string argstr = argument;
//  int speed;
  float hx, hy, hz;

  argstr = one_argument( argstr , arg);
  argstr = one_argument( argstr , arg1);
  argstr = one_argument( argstr , arg2);
  argstr = one_argument( argstr , arg3);
  
    if ( (ship = ship_from_cockpit(ch->game, ch->in_room->vnum)) == NULL )
    {
        send_to_char("&RYou must be in the cockpit of a ship to do that!\n",ch);
        return;
    }

    if ( ship->shipclass > SHIP_PLATFORM )
    {
        send_to_char("&RThis isn't a spacecraft!",ch);
        return;
    }

    if ( !ship->spaceobject )
    {
        send_to_char("&RYou can only do that in space!\n",ch);
        return;
    }

  if( !str_cmp( arg, "dist" ) )
  {
    ship->tcount = strtoi(arg1);
    send_to_char("&RJump distance set!\n",ch);
    return;
  }
    
  if( !str_cmp( arg, "set" ) )
  {
    if (ship->shipstate == SHIP_HYPERSPACE )
    {
        send_to_char("&RYou can only do that in realspace!\n",ch);
        return;
    }

    if( !is_number(arg1) || !is_number(arg2) || !is_number(arg3) )
    {
      send_to_char( "Syntax: shiptrack set <X Heading> <Y Heading> <Z Heading>.\n", ch);
      return;
    }
    
      hx = strtoi(arg1);
      hy = strtoi(arg2);
      hz = strtoi(arg3);
      if( !hx )
       hx = 1;
      if( !hy )
       hy = 1;
      if( !hz )
       hz = 1;
      buf = str_printf( "%.0f %.0f %.0f", ship->vx + hx, ship->vy + hy, ship->vz + hz );
      if( hx < 1000 ) hx *= 10000;
      if( hy < 1000 ) hy *= 10000;
      if( hz < 1000 ) hz *= 10000;


      ship->tx = hx;
      ship->ty = hy;
      ship->tz = hz;
 
      ship->tracking = TRUE;
      ship->ch = ch;
      do_trajectory( ch, (char*)buf.c_str());
      
//    speed = ship->mod->hyperspeed;

    ship->jx = ship->vx + hx;
    ship->jy = ship->vy + hy;
    ship->jz = ship->vz + hz;

    for( spaceobject = first_spaceobject; spaceobject; spaceobject = spaceobject->next )
      if( space_in_range( ship, spaceobject ) )
      {
        ship->currjump = spaceobject;
        break;
      }
    if( !spaceobject )
      ship->currjump = ship->spaceobject;

    if( ship->jx > 15000000 || ship->jy > 15000000 || ship->jz > 15000000 ||
        ship->jx < -15000000 || ship->jy < -15000000 || ship->jz < -15000000 ||
        ship->vx > 15000000 || ship->vy > 15000000 || ship->vz > 15000000 ||
        ship->vx < -15000000 || ship->vy < -15000000 || ship->vz < -15000000 ||
        ship->hx > 15000000 || ship->hy > 15000000 || ship->hz > 15000000 ||
        ship->hx < -15000000 || ship->hy < -15000000 || ship->hz < -15000000 )
    {
                    echo_to_cockpit( AT_RED, ship, "WARNING.. Jump coordinates outside of the known galaxy.");
                    echo_to_cockpit( AT_RED, ship, "WARNING.. Hyperjump NOT set.");
                    ship->currjump = NULL;
                    ship->tracking = FALSE;
                    return;
    }


    ship->hyperdistance  = abs( (int) ( ship->vx - ship->jx )) ;
    ship->hyperdistance += abs( (int) ( ship->vy - ship->jy )) ;
    ship->hyperdistance += abs( (int) ( ship->vz - ship->jz )) ;
    ship->hyperdistance /= 50;

    ship->orighyperdistance = ship->hyperdistance;

    send_to_char( "Course laid in. Beginning tracking program.\n", ch);

    return;
  }
  if( !str_cmp( arg, "stop" ) || !str_cmp( arg, "halt" ))
  {
    ship->tracking = FALSE;
    send_to_char( "Tracking program cancelled.\n", ch);
    if( ship->shipstate == SHIP_HYPERSPACE )
      do_hyperspace( ch, "off" );
  }
}

void do_transship(CHAR_DATA *ch, char *argument)
{
  std::string arg1;
	std::string arg2;
  std::string argstr = argument;
	int arg3, origShipyard;
    SHIP_DATA *ship;
    
    if ( IS_NPC( ch ) )
    {
	send_to_char( "Huh?\n", ch );
	return;
    }

    argstr = one_argument( argstr, arg1 );
    argstr = one_argument( argstr, arg2 );

    ship = get_ship( ch->game, arg1 );
    if ( !ship )
    {
        send_to_char( "No such ship.\n", ch );
        return;
    }

	  arg3 = strtoi( arg2 );
	
	  if ( arg1.empty() || arg2.empty() || arg3 == 0 )
    {
        send_to_char( "Usage: transship <ship> <vnum>\n", ch );
        return;
    }

    origShipyard = ship->shipyard;
    
    ship->shipyard = arg3;
    ship->shipstate = SHIP_READY;

    if ( ship->shipclass == SHIP_PLATFORM && ship->type != MOB_SHIP )
    {
        send_to_char( "Only nonmob midship/starfighters", ch );
        return;
    }
     
    extract_ship( ship );
    ship_to_room( ship , ship->shipyard ); 
    
    ship->location = ship->shipyard;
    ship->lastdoc = ship->shipyard; 
    ship->shipstate = SHIP_LANDED;
    ship->shipyard = origShipyard;     
    
    if (ship->spaceobject)
      ship_from_spaceobject( ship, ship->spaceobject );  
    
    save_ship(ship);              
    
    send_to_char( "Ship Transfered.\n", ch );
}

void transship(SHIP_DATA *ship, int destination)
{
    int origShipyard;
   

    if ( !ship )
	      return;

     origShipyard = ship->shipyard;
     
     ship->shipyard = destination;
     ship->shipstate = SHIP_READY;

     extract_ship( ship );
     ship_to_room( ship , ship->shipyard ); 
     
     ship->location = ship->shipyard;
     ship->lastdoc = ship->shipyard; 
     ship->shipstate = SHIP_LANDED;
     ship->shipyard = origShipyard;
     
     if (ship->spaceobject)
        ship_from_spaceobject( ship, ship->spaceobject );  
     
     save_ship(ship);               
}

void do_override(CHAR_DATA *ch, char *argument)
{
	
    std::string arg;
    std::string arg2;
    std::string buf;
    SHIP_DATA *ship;
    SHIP_DATA *eShip = NULL;

    arg2 = one_argument( argument, arg );

    if ( (ship = ship_from_cockpit(ch->game, ch->in_room->vnum)) == NULL )
    {
        send_to_char("&RYou must be in the cockpit of a ship to do that!\n",ch);
        return;
    }

    if ( ship->shipclass > SHIP_PLATFORM )
    {
        send_to_char("&RThis isn't a spacecraft!",ch);
        return;
    }

    if ( !ship->spaceobject )
    {
        send_to_char("&RYou can't do that until you've finished launching!\n",ch);
        return;
    }

    if (ship->shipstate == SHIP_HYPERSPACE )
    {
        send_to_char("&RYou can only do that in realspace!\n",ch);
        return;
    }

    if ( arg.empty() )
    {
        send_to_char("&ROverride the controls of what ship?\n", ch);
        return;
    }

    eShip = get_ship_here(ship->game, arg,ship);

    if ( eShip == NULL )
    {
        send_to_char("&RThat ship isn't here!\n",ch);
        return;
    }

    if ( eShip == ship )
    {
        send_to_char("&RYou are in the cockpit, just hit the controls!\n", ch);
        return;
    }

    if ( abs( (int) ( eShip->vx - ship->vx )) > 100*((ship->mod->comm)+(eShip->mod->comm)+20) ||
    abs( (int) ( eShip->vy - ship->vy )) > 100*((ship->mod->comm)+(eShip->mod->comm)+20) ||
    abs( (int) ( eShip->vz - ship->vz )) > 100*((ship->mod->comm)+(eShip->mod->comm)+20) )

    {
      send_to_char("&RThat ship is out of the range of your comm system.\n&w", ch);
      return;
    }


    if ( !check_pilot(ch,eShip) )
    {
        send_to_char("&RHey! That's not your ship!",ch);
        return;
    }

    if ( !str_cmp( arg2, "shields" ) )
    {
      if( eShip->shield == 0 )
      {
          eShip->autorecharge=TRUE;
          send_to_char( "&GShields on. Confirmed.\n", ch);
          echo_to_cockpit( AT_YELLOW , eShip , "Shields ON. Autorecharge ON.");
          return;
      }
      else
      {
          eShip->shield = 0;
          eShip->autorecharge=FALSE;
          send_to_char("Shields down. Confirmed", ch);
          return;
      }
    }
    if ( !str_cmp( arg2, "closebay" ) )
    {
        eShip->bayopen=FALSE;
        send_to_char( "&GBays Close. Confirmed.\n", ch);
        echo_to_cockpit( AT_YELLOW , eShip , "Bays Closed");
        buf = str_printf("%s's bay doors close." , eShip->name );
        echo_to_system( AT_YELLOW, eShip, buf , NULL );
        return;
    }

    if ( !str_cmp( arg2, "stop" ) )
    {
        eShip->goalspeed = 0;	
        eShip->accel = get_acceleration(eShip);
        send_to_char( "&GBraking Thrusters. Confirmed.\n", ch);
        echo_to_cockpit( AT_GREY , eShip , "Braking thrusters fire and the ship stops");
        buf = str_printf("%s decelerates." , eShip->name );
        echo_to_system( AT_GREY, eShip, buf , NULL );
        return;
    }
    
    if ( !str_cmp( arg2, "autopilot" ) )
    {
      if ( ship->autopilot )
      {
          eShip->autopilot=FALSE;
          send_to_char( "&GYou toggle the autopilot.\n", ch);
          echo_to_cockpit( AT_YELLOW , eShip , "Autopilot OFF.");
          return;
      }      
      else if ( !ship->autopilot )
      {
          eShip->autopilot=TRUE;
          send_to_char( "&GYou toggle the autopilot.\n", ch);
          echo_to_cockpit( AT_YELLOW , eShip , "Autopilot ON.");
          return;
      }
    }

    if ( !str_cmp( arg2, "openbay" ) )
    {
        send_to_char("&RYou open the bay doors of the remote ship.",ch);
        act(AT_PLAIN,"$n flips a switch on the control panel.",ch,NULL,argument,TO_ROOM);
        eShip->bayopen = TRUE;
        buf = str_printf("%s's bay doors open." , eShip->name );
        echo_to_system( AT_YELLOW, ship, buf , NULL );
        return;
    }
    
    send_to_char("Choices: shields - Toggle shields   autopilot - Toggle autopilot\n", ch);
    send_to_char("         openbay   closebay  stop  \n", ch);
    return;
}

void do_guard( CHAR_DATA *ch, char *argument )
{
    int chance;
    SHIP_DATA *ship;


    if (  (ship = ship_from_cockpit(ch->game, ch->in_room->vnum))  == NULL )
    {
        send_to_char("&RYou must be in the cockpit of a ship to do that!\n",ch);
        return;
    }
    
    if (  (ship = ship_from_pilotseat(ch->game, ch->in_room->vnum))  == NULL )
    {
        send_to_char("&RYou must be in the pilots seat!\n",ch);
        return;
    }
    
    if ( ship->shipclass != CAPITAL_SHIP  && ship->shipclass != SHIP_PLATFORM )
          {
              send_to_char("&ROnly capital-class vessels and platforms have this feature.\n",ch);
              return;
          }
          
    chance = IS_NPC(ch) ? ch->top_level
          : (int)  (ch->pcdata->learned[gsn_shipsystems]) ;
    if ( number_percent( ) > chance )
    {
        send_to_char("&RYou fail to work the controls properly.\n",ch);
        learn_from_failure( ch, gsn_shipsystems );
        return;	
    }
    
    act( AT_PLAIN, "$n flips a switch on the control panell.", ch,
         NULL, argument , TO_ROOM );

    if ( !str_cmp(argument,"on" ) )
    {
        ship->guard=TRUE;
        send_to_char( "&GYou activate the guard system.\n", ch);
        echo_to_cockpit( AT_YELLOW , ship , "Guard System: ACTIVATED.");
        ship->goalspeed = 0;
        ship->accel = get_acceleration( ship );
    }
    else if ( !str_cmp(argument,"off" ) )
    {
        ship->guard=FALSE;
        send_to_char( "&GYou shutdown the guard system.\n", ch);
        echo_to_cockpit( AT_YELLOW , ship , "Guard System: DEACTIVATED.");
    }
    else
    {   
        if (ship->guard == TRUE)
        {
          ship->guard=FALSE;
          send_to_char( "&GYou shutdown the guard system.\n", ch);
          echo_to_cockpit( AT_YELLOW , ship , "Guard System: DEACTIVATED.");
        }
        else
        {
          ship->guard=TRUE;
          send_to_char( "&GYou activate the guard system.\n", ch);
          echo_to_cockpit( AT_YELLOW , ship , "Guard System: ACTIVATED.");
          ship->goalspeed = 0;
          ship->accel = get_acceleration( ship );
        }   
    }

    learn_from_success( ch, gsn_shipsystems );    	
    return;
}

void do_sabotage(CHAR_DATA *ch, char *argument )
{
    std::string arg;
//  char buf[MAX_INPUT_LENGTH];
    int chance, change;
    SHIP_DATA *ship;

    arg = argument;

    switch( ch->substate )
    {
    	default:
          if (  (ship = ship_from_engine(ch->game, ch->in_room->vnum))  == NULL )
          {
              send_to_char("&RYou must be in the engine room of a ship to do that!\n",ch);
              return;
          }

          if ( str_cmp( argument , "hull" ) && str_cmp( argument , "drive" ) &&
                str_cmp( argument , "launcher" ) && str_cmp( argument , "laser" ) &&
                str_cmp( argument , "docking" ) && str_cmp( argument , "tractor" ) )
          {
              send_to_char("&RYou need to specify something to sabotage:\n",ch);
              send_to_char("&rTry: hull, drive, launcher, laser, docking, or tractor.\n",ch);
              return;
          }

          chance = IS_NPC(ch) ? ch->top_level
              : (int) (ch->pcdata->learned[gsn_sabotage]);
          if ( number_percent( ) < chance )
    		  {
            send_to_char( "&GYou begin your work.\n", ch);
            act( AT_PLAIN, "$n begins working on the ship's $T.", ch,
              NULL, argument , TO_ROOM );
            if ( !str_cmp(arg,"hull") )
                add_timer ( ch , TIMER_DO_FUN , 15 , do_sabotage , 1 );
            else
                add_timer ( ch , TIMER_DO_FUN , 15 , do_sabotage , 1 );
            ch->dest_buf = str_dup(arg);
            return;
	        }
	        send_to_char("&RYou fail to figure out where to start.\n",ch);
	        learn_from_failure( ch, gsn_sabotage );
    	   	return;

    	case 1:
          if ( !ch->dest_buf )
            return;
          arg = ( const char* ) ch->dest_buf;
          STR_DISPOSE( ch->dest_buf);
          break;

    	case SUB_TIMER_DO_ABORT:
          STR_DISPOSE( ch->dest_buf );
          ch->substate = SUB_NONE;
          if ( (ship = ship_from_cockpit(ch->game, ch->in_room->vnum)) == NULL )
              return;
          send_to_char("&RYou are distracted and fail to finish your work.\n", ch);
          return;
    }

    ch->substate = SUB_NONE;
    
    if ( (ship = ship_from_engine(ch->game, ch->in_room->vnum)) == NULL )
    {  
        return;
    }
    
    if ( !str_cmp(arg,"hull") )
    {
        change = URANGE( 0 , 
                  number_range( (int) ( ch->pcdata->learned[gsn_sabotage] / 2 ) , (int) (ch->pcdata->learned[gsn_sabotage]) ),
                  ( ship->hull ) );
        ship->hull -= change;
        ch_printf( ch, "&GSabotage complete.. Hull strength decreased by %d points.\n", change );
    }
    
    if ( !str_cmp(arg,"drive") )
    {  
        if (ship->location == ship->lastdoc)
            ship->shipstate = SHIP_DISABLED;
        else if ( ship->shipstate == SHIP_HYPERSPACE )
            send_to_char("You realize after working that it would be a bad idea to do this while in hyperspace.\n", ch);		
        else     
            ship->shipstate = SHIP_DISABLED;
        send_to_char("&GShips drive damaged.\n", ch);		
    }

    if ( !str_cmp(arg,"docking") )
    {  
        ship->statetdocking = SHIP_DISABLED;
        send_to_char("&GDocking bay sabotaged.\n", ch);
      }
    if ( !str_cmp(arg,"tractor") )
    {  
        ship->statettractor = SHIP_DISABLED;
        send_to_char("&GTractorbeam sabotaged.\n", ch);
    }
    if ( !str_cmp(arg,"launcher") )
    {  
        ship->missilestate = MISSILE_DAMAGED;
        send_to_char("&GMissile launcher sabotaged.\n", ch);
    }
    
    if ( !str_cmp(arg,"laser") )
    {  
        ship->statet0 = LASER_DAMAGED;
        send_to_char("&GMain laser sabotaged.\n", ch);
    }

    act( AT_PLAIN, "$n finishes the work.", ch,
         NULL, arg , TO_ROOM );

    bug( "%s has sabotaged %s!", ch->name, ship->name );

    learn_from_success( ch, gsn_sabotage );
    	
}

void do_refuel(CHAR_DATA *ch, char *argument )
{
}

void do_fuel(CHAR_DATA *ch, char *argument )
{
    SHIP_DATA *ship, *eShip = NULL;
    int amount = 0;
    std::string arg1;
    std::string buf;
    
    
    one_argument( argument, arg1 );
    
    if (  (ship = ship_from_hanger(ch->game, ch->in_room->vnum))  == NULL )
    {
      if ( (ship = ship_from_entrance(ch->game, ch->in_room->vnum)) == NULL )
      {
        send_to_char("&RYou must be in the hanger or the entrance of a ship to do that!\n",ch);
        return;
      }
    }

    if( /* arg2[0] == '\0' || */arg1.empty() || !is_number(arg1) )
    {
      send_to_char( "Syntax: Fuel <amount> <ship>", ch);
      return;
    }
    

    {
      if( !ship->docked )
      {
        for( eShip = first_ship; eShip; eShip = eShip->next )
          if( eShip->docked && eShip->docked == ship )
            break;
      }
      else
        eShip = ship->docked;
    }

  /*  if( !eShip )
    {
        eShip = ship_in_room( ch->in_room, argument );

    if( !eShip )
    {
      eShip = get_ship( argument );
      if( eShip && (!eShip->docked || eShip->docked != ship ) )
        eShip = NULL;
    }
    }
  */
    if( !eShip )
    {
      send_to_char( "Ship not docked. Fuel what ship?", ch );
      return;
    }    
      
    amount = strtoi(arg1);
    
    if( amount >= ship->energy )
    {
      send_to_char( "&RError: Ordered energy over current stock. Sending everything but 1 unit.\n", ch );
      amount = ship->energy - 1;
    }
    
    if( amount + eShip->energy > eShip->mod->maxenergy )
    {
      send_to_char( "&rError: Ordered energy over target capacity. Filling tanks.\n", ch );
      amount = eShip->mod->maxenergy - eShip->energy;
    }
     
    if( ship->shipclass != SHIP_PLATFORM )
      ship->energy -= amount;

    eShip->energy += amount;
    
    buf = "&YFuel order filled: &O" + std::string(eShip->name) + ": " + std::to_string(amount) + "\n";
    echo_to_cockpit( AT_YELLOW, ship, buf );
    send_to_char( buf, ch );
    buf = "&YFuel remaining: " + std::to_string(ship->energy) + "\n";
    echo_to_cockpit( AT_YELLOW, ship, buf );
    send_to_char( buf, ch );      
    
}


void do_renameship( CHAR_DATA *ch, char *argument )
{
   SHIP_DATA *ship;
   CLAN_DATA *clan;
      if ( (ship = ship_from_cockpit( ch->game, ch->in_room->vnum ) ) == NULL)
      {            
           send_to_char( "You must be in the cockpit of a ship to do that!\n", ch);
           return;
      }
      
      if( ( (clan = get_clan(ship->owner)) == NULL ) || str_cmp( clan->leader, ch->name ) )
        if( str_cmp( ship->owner, ch->name ) )
        {            
           send_to_char( "&RImperial Database: &WYou do not own this ship.\n", ch);
           return;
        }

      if( get_ship( ch->game, argument ) != NULL )
        {            
           send_to_char( "&RImperial Database: &WA ship already exists of that name.\n", ch);
           return;
        }
      
      if( strchr( argument, '&' ) != NULL )
        {            
           send_to_char( "&RColors may not be used in a ship's name.\n", ch);
           return;
        }
      
      if( ch->gold < 50000 )
      {            
           send_to_char( "&RImperial Database: &WYou do not have enough credits for this request.\n", ch);
           return;
      }
        

      ch->gold -= 50000;
      STRFREE( ship->personalname );
      ship->personalname		= STRALLOC( argument );
      save_ship( ship );      
      send_to_char( "&RImperial Database: &WTransaction Complete. Name changed.", ch );

}

long get_distance_from_ship( SHIP_DATA *ship, SHIP_DATA *target )
{
  long hx, hy, hz;
  
  hx = abs( (int) ( target->vx - ship->vx ));
  hy = abs( (int) ( target->vy - ship->vy ));
  hz = abs( (int) ( target->vz - ship->vz ));
  
  return hx+hy+hz;
}

void target_ship( SHIP_DATA *ship, SHIP_DATA *target )
{
  char buf[MAX_STRING_LENGTH];

        ship->target0 = target;
        SPRINTF( buf , "You are being targetted by %s." , ship->name);
        echo_to_cockpit( AT_BLOOD , target , buf );
        SPRINTF( buf , "The ship targets %s." , target->name);
        echo_to_cockpit( AT_BLOOD , ship , buf );
}

bool check_hostile( SHIP_DATA *ship )
{
  long distance = -1, tempdistance;
  SHIP_DATA *target;
  SHIP_DATA *enemy = NULL;
//char buf[MAX_STRING_LENGTH];

  if ( !autofly(ship) || ship->shipclass == SHIP_DEBRIS )
    return FALSE;
    
  for( target = first_ship; target; target = target->next )
  {
    if( !ship_in_range( ship, target ) )
      continue;
      
    if ( !str_cmp( ship->owner , "The Empire" ) )
    {
      if ( !str_cmp( target->owner , "The Rebel Alliance" ) || !str_cmp( target->owner , "The New Republic"))
      {
        tempdistance = get_distance_from_ship( ship, target );
	if( distance == -1 || distance > tempdistance )
	{
	  distance = tempdistance;
	  enemy = target;
	}
      }
    }
    if ( (!str_cmp( ship->owner , "The Rebel Alliance" )) || (!str_cmp( ship->owner , "The New Republic" )))
    {
      if ( !str_cmp( target->owner , "The Empire" ) )
      {
        tempdistance = get_distance_from_ship( ship, target );
	if( distance == -1 || distance > tempdistance )
	{
	  distance = tempdistance;
	  enemy = target;
	}
      }
    }
    if ( !str_cmp( ship->owner , "Pirates" ) )
    {
      if ( str_cmp(target->owner, "Pirates") )
      {
        tempdistance = get_distance_from_ship( ship, target );
	if( distance == -1 || distance > tempdistance )
	{
	  distance = tempdistance;
	  enemy = target;
	}
      }
    }
    if ( !str_cmp( ship->owner , "Zsinj" ) )
    {
      if ( str_cmp(target->owner, "Zsinj") )
      {
        tempdistance = get_distance_from_ship( ship, target );
	if( distance == -1 || distance > tempdistance )
	{
	  distance = tempdistance;
	  enemy = target;
	}
      }
    }
    if ( !str_cmp( ship->owner , "Empire SpecOps" ) )
    {
      if ( str_cmp(target->owner, "Empire SpecOps") && 
		           str_cmp(target->owner, "The Empire") &&
		            target->type != SHIP_IMPERIAL )
      {
        tempdistance = get_distance_from_ship( ship, target );
	if( distance == -1 || distance > tempdistance )
	{
	  distance = tempdistance;
	  bug("%ld %ld %s %s", distance, tempdistance, ship->name, target->name );
	  enemy = target;
	}
      }
    }

  }
  
  if ( enemy )
  {
    target_ship( ship, enemy );
    return TRUE; 
  }
  return FALSE;
  
}	

void fread_modules( SHIP_DATA *ship, FILE *fp )
{
	const char * word;
	MODULE_DATA * module;
	TURRET_DATA * turret;
        char *line;
	int x1, x2, x3, x4;
//	bool found = FALSE;

	ship->modules = 0;

        CREATE( module, MODULE_DATA, 1 );
	CREATE( turret, TURRET_DATA, 1 );
	
	for( ; ; )
	{
	  word = feof( fp ) ? "End" : fread_word( fp );
      if( !str_cmp( word, "End" ) )
        break;

	    if ( !str_cmp( word, "Name" ) )
	    {
//	    	found = TRUE;
		module->name = fread_string( fp );
//		log_string( "NAME:" );
//		log_string( module->name );
	    }
  	    if ( !str_cmp( word, "Mod" ) )
	    {
//	    	found = TRUE;
		line = fread_string_nohash( fp );
/*		log_string( "MOD:" );
		log_string( line );
*/
		x1=x2=x3=x4=0;
		sscanf( line, "%d %d %d %d",
		      &x1, &x2, &x3, &x4 );
		free( line );
		if( x1 != MOD_TURRET )
		{
		  module->type 		= x1;
		  module->condition 	= x2;
		  module->size		= x3;
		  module->modification	= x4;

		  LINK( module, ship->first_module, ship->last_module, next, prev );
		  ship->modules++;
		  DISPOSE( turret );
		}
		if ( x1 == MOD_TURRET )
		{
		  // = x1; - x1 set to MOD_TURRET - DV 8/8/02
		  turret->type	 	= x2;
		  if ( ship->shipclass == MIDSIZE_SHIP && !(turret->type) )
		    turret->type = QUAD_TURRET;
		  turret->roomvnum	= x3;
		  turret->state		= x4;

		  LINK( turret, ship->first_turret, ship->last_turret, next, prev );
		  DISPOSE( module );
		}
	    }
	 }
	 
	 update_ship_modules(ship);
	 
	 return;
}

bool is_ammo_mod( int type )
{
  if( type == MOD_MISSILE || type == MOD_TORPEDO || type == MOD_ROCKET || type == MOD_CHAFF )
    return TRUE;
    
  return FALSE;
}
	
int get_extmodule_count( SHIP_DATA *ship )
{
  MODULE_DATA *module;
  int external = 0;
//int internal = 0;
  
  for ( module = ship->first_module; module; module = module->next )
    {
      if( module->type == MOD_HULL )
        continue;
      else if( is_external_mod( module->type ) )
	      external += module->size;
      else if( is_internal_mod( module->type ) )
      	continue;
    }

return external;

}
	
int get_intmodule_count( SHIP_DATA *ship )
{
  MODULE_DATA *module;
//int external = 0;
  int internal = 0;
  
  for ( module = ship->first_module; module; module = module->next )
    {
      if( module->type == MOD_HULL )
        continue;
      else if( is_external_mod( module->type ) )
	      continue;
      else if( is_internal_mod( module->type ) )
	      internal += module->size;
    }

return internal;

}

void updateship( SHIP_DATA *ship, int type )
{
	if( !ship || type <= 0 )
	  return;
	  
	if( type == MOD_HULL )
	  ship->hull = ship->mod->maxhull;
	if( type == MOD_ENERGY )
	  ship->energy = ship->mod->maxenergy;
	  
	return;
}


void update_ship_modules( SHIP_DATA *ship )
{

	MODULE_DATA *module;
	SHIP_MOD_DATA *mod;
	
	if( !ship->mod )
	{
	  CREATE( mod, SHIP_MOD_DATA, 1 );
	  ship->mod = mod;
	}

	ship->mod->hyperspeed = ship->hyperspeed;
	ship->mod->realspeed = ship->realspeed;
	ship->mod->maxshield = ship->maxshield;
	ship->mod->lasers = ship->lasers;
	ship->mod->ions = ship->ions;
	ship->mod->tractorbeam = ship->tractorbeam;
	
	
	ship->mod->maxenergy = ship->maxenergy;
	ship->mod->comm = ship->comm;
	ship->mod->sensor = ship->sensor;
	ship->mod->astro_array = ship->astro_array;
	if( ship->chaff )
	  ship->mod->defenselaunchers = 1;
	else
	  ship->mod->defenselaunchers = 0;
	ship->mod->launchers = 0;
	ship->mod->manuever = ship->manuever;
	ship->mod->gravitypower = 0;
	ship->mod->maxhull = ship->maxhull;
	ship->modules = 0;

	for ( module=ship->first_module; module; module = module->next )
	{
			if ( module->type == MOD_HYPERSPEED && ( (module->modification*module->condition/MOD_CONDITION_START) > ship->mod->hyperspeed ) )
				ship->mod->hyperspeed = ( module->modification*(module->condition/MOD_CONDITION_START ) );
			if ( module->type == MOD_REALSPEED )
				ship->mod->realspeed+= ( module->modification*(module->condition/MOD_CONDITION_START ) );
			if ( module->type == MOD_MAXSHIELD )
				ship->mod->maxshield += ( module->modification*(module->condition/MOD_CONDITION_START ) );
			if ( module->type == MOD_LASER )
				ship->mod->lasers += ( module->modification );
			if ( module->type == MOD_ION )
				ship->mod->ions += ( module->modification );
			if ( module->type == MOD_TRACTORBEAM )
				ship->mod->tractorbeam += ( module->modification*(module->condition/MOD_CONDITION_START ) );
			if ( module->type == MOD_LAUNCHER )
				ship->mod->launchers += 1;
			if ( module->type == MOD_ENERGY )
				ship->mod->maxenergy += ( module->modification*(module->condition/MOD_CONDITION_START ) );
			if ( module->type == MOD_COMM )
				ship->mod->comm += ( module->modification*(module->condition/MOD_CONDITION_START ) );
			if ( module->type == MOD_SENSOR )
				ship->mod->sensor+= ( module->modification*(module->condition/MOD_CONDITION_START ) );
			if ( module->type == MOD_ASTRO_ARRAY)
				ship->mod->astro_array += ( module->modification*(module->condition/MOD_CONDITION_START ) );
			if ( module->type == MOD_DEFENSELAUNCHER )
				ship->mod->defenselaunchers += 1;
			if ( module->type == MOD_MANUEVER )
				ship->mod->manuever += ( module->modification*(module->condition/MOD_CONDITION_START ) );
			if ( module->type == MOD_GRAVITY_PROJ )
				ship->mod->gravitypower += ( module->modification*(module->condition/MOD_CONDITION_START ) );
			if ( module->type == MOD_HULL )
				ship->mod->maxhull += ( module->modification*(module->condition/MOD_CONDITION_START ) );
		        if ( module->type == MOD_FLAG )
	 		{
	   		  ROOM_INDEX_DATA *froom;
	   		  if ( (froom = get_room_index(module->condition)) != NULL )
	     		    BV_SET_BIT( froom->room_flags, modflags[module->modification] );
	 		}
				
			ship->modules++;
	}

	if( ship->missiles || ship->torpedos || ship->rockets )
	  if( !ship->mod->launchers )
	    ship->mod->launchers = 1;


	return;
}




void do_install_module( CHAR_DATA *ch, char *argument )
{
    std::string arg1;
    std::string arg2;
    std::string buf;
    std::string argstr = argument;
    OBJ_DATA *obj;
    SHIP_DATA *ship = NULL;
    MODULE_DATA *module;    
    int chance;

    argstr = one_argument( argstr, arg1 );
    argstr = one_argument( argstr, arg2 );

    chance = IS_NPC(ch) ? ch->top_level
             : (int) (ch->pcdata->learned[gsn_shipmaintenance]);

    if( chance <= 0 )
    {
        send_to_char( "You do not know how to install a module.\n", ch );
        return;
    }
    
    if ( arg1.empty() )
    {
        send_to_char( "Install what?\n", ch );
        return;
    }

    if ( ms_find_obj(ch) )
      return;

    if ( ( obj = get_obj_carry( ch, arg1 ) ) == NULL )
    {
        send_to_char( "You do not have that item.\n", ch );
        return;
    }


    if ( !( obj->item_type==ITEM_FIGHTERCOMP || obj->item_type==ITEM_MIDCOMP
      || obj->item_type==ITEM_CAPITALCOMP ) )
    {
        send_to_char("That isn't a ship module.\n",ch);
        return;
    }


    if ( !can_drop_obj( ch, obj ) )
    {
        send_to_char( "You can't let go of it.\n", ch );
        return;
    }

    if ( obj->value[1] == MOD_FLAG )
    {
      if ( (ship = ship_from_room( ch->game, ch->in_room->vnum )) == NULL )
      {
          send_to_char( "You need to be in the room you want to install this item!\n", ch );
          return;
      }
      
      if ( obj->value[3] >= MAXMODFLAG )
      {
          send_to_char( "That module can not be installed!  Contact an administrator.\n", ch );
          bug( "Module's flag is set beyond the array limits! Char: %s Module vnum: %d\n",
              ch->name, obj->pIndexData->vnum);
          return;
      }

      obj->value[0] = ch->in_room->vnum;
    }
      

    if ( arg2.empty() && !ship )
    {
      if( ( ship = ship_from_engine( ch->game, ch->in_room->vnum ) ) == NULL )
      {
          send_to_char( "Install what in what?\n", ch );
          return;
      }
    }

    if ( !ship )
      ship = ship_in_room( ch->in_room , arg2 );    
      
    if ( !ship )            
    {    
        act( AT_PLAIN, "I see no $T here.", ch, NULL, argument, TO_CHAR );        
        return;       
    }

//	SPRINTF( buf, "Ship: %s\n", ship->personalname );
//	send_to_char( buf, ch );

  if ( obj->value[1] != MOD_FLAG )
  {
      if ( obj->item_type==ITEM_FIGHTERCOMP && ship->shipclass != FIGHTER_SHIP )
      {
          send_to_char( "That module is designed for a fighter.\n",ch);
          return;
      }

      if ( obj->item_type==ITEM_MIDCOMP && ship->shipclass != MIDSIZE_SHIP )
      {
          send_to_char( "That module is designed for a midship.\n",ch);
          return;
      }

      if ( obj->item_type==ITEM_CAPITALCOMP && ship->shipclass != CAPITAL_SHIP )
      {
          send_to_char( "That module is designed for a capital ship.\n",ch);
          return;
      }
  }
  if ( obj->value[1] == MOD_FLAG )
    if ( ship == ship_from_cockpit( ch->game, ch->in_room->vnum ) )
    {
        send_to_char( "You can not place this in a control room of a ship.\n",ch);
        return;
    }
        
      
	if ( !IS_GOD(ch) && ( ship->shipclass != SHIP_DEBRIS ) && ( !check_pilot( ch , ship ) || !str_cmp( ship->owner , "Public" ) ) )
	{    
      send_to_char("&RHey, thats not your ship!\n",ch);    	
      return;    	
	}

	if ( module_type_install(obj, ship) )
	{
      send_to_char( "There is no room for that part!\n", ch);
      return;
	}

	if( obj->value[1] == MOD_FLAG )
	{
      if ( BV_IS_SET( ch->in_room->room_flags, modflags[obj->value[3]] ) )
      {
        send_to_char( "&RThis item is already placed here.\n", ch);
        return;
      }
  }

  if ( number_percent( ) > chance )
	{
      send_to_char( "You fail to figure out how to install this module.\n", ch );
      return;
  }

	if( obj->value[1] == MOD_MISSILE )
	    ship->missiles += obj->value[3];

	if( obj->value[1] == MOD_TORPEDO )
	    ship->torpedos+= obj->value[3];

	if( obj->value[1] == MOD_ROCKET )
	    ship->rockets += obj->value[3];

	if( obj->value[1] == MOD_CHAFF)
	    ship->chaff += obj->value[3];

	separate_obj( obj );

	act( AT_ACTION, "$n installs $p.", ch, obj, NULL, TO_ROOM );
	act( AT_ACTION, "You install $p.", ch, obj, NULL, TO_CHAR );

  if( !is_ammo_mod( obj->value[1] ) )
  {
      ship->modules += 1;
      
      CREATE( module, MODULE_DATA, 1 );
      
      buf = str_printf( "%s", obj->short_descr );
      module->name =  STRALLOC( buf );
      module->type = obj->value[1];
      module->condition = obj->value[0];
      module->size = obj->value[2];
      module->modification = obj->value[3];
      
      LINK( module, ship->first_module, ship->last_module, next, prev );

      update_ship_modules( ship );
      updateship( ship, module->type );
  }

	extract_obj(obj);
	save_ship( ship );
	  
  return;

}

bool is_internal_mod( int type )
{
	switch(type)
	{
	  case MOD_COMM:
	  case MOD_SENSOR:
	  case MOD_ASTRO_ARRAY:
	    return TRUE;
	  default: return FALSE;
	}
}

bool is_external_mod( int type )
{
      switch( type )
      {
  	case MOD_HYPERSPEED:
  	case MOD_REALSPEED:
  	case MOD_LASER:
  	case MOD_ION:
  	case MOD_MAXSHIELD:
  	case MOD_ENERGY:
  	case MOD_LAUNCHER:
    	case MOD_TRACTORBEAM:
  	case MOD_DEFENSELAUNCHER:
  	case MOD_MANUEVER:
  	case MOD_GRAVITY_PROJ:
  	case MOD_CARGO:
	  return TRUE;
	default: return FALSE;  
      }
      
      return FALSE;
}

char *show_mod_type( MODULE_DATA *module )
{
  if( !module )
    return "(null)";
  return show_mod_type2 ( module->type );
}

char *show_mod_type2( int type )
{
  
  switch( type )
  {
  	case MOD_HYPERSPEED:  return "Hyperdrive"; break;
  	case MOD_REALSPEED:  return "Realspeed"; break;
  	case MOD_LASER:  return "Laser"; break;
  	case MOD_ION:  return "Ion"; break;
  	case MOD_MAXSHIELD:  return "Shield"; break;
  	case MOD_ENERGY:  return "Fuel Pod"; break;
  	case MOD_LAUNCHER:  return "Projectile Launcher"; break;
    	case MOD_TRACTORBEAM:  return "Tractorbeam"; break;
  	case MOD_COMM:  return "Communication System"; break;	
  	case MOD_SENSOR:  return "Sensor Package"; break;
  	case MOD_ASTRO_ARRAY:  return "Astronavigation System"; break;
  	case MOD_DEFENSELAUNCHER:  return "Defense System"; break;
  	case MOD_MANUEVER:  return "Manuevering Jets"; break;
  	case MOD_MISSILE:  return "Missile Launcher"; break;
  	case MOD_TORPEDO:  return "Torpedo Launcher"; break;
  	case MOD_ROCKET:  return "Rocket Launcher"; break;
  	case MOD_CHAFF:  return "Defense Launcher"; break;
  	case MOD_GRAVITY_PROJ:  return "Gravity Well Projector"; break;
  	case MOD_HULL:  return "Hull Plating"; break;
  	case MOD_TURRET: return "Turret"; break;
  	case MOD_CARGO: return "Cargo"; break;
  	case MOD_FLAG: return "Flag"; break;
  	default: return "Unknown"; break;
  	
  	// UPDATE show_mod_type3 - DV 5-15-04
  }
}

// Added for show_mod <type> - DV 5-15-04
int show_mod_type3( const std::string& type )
{
  
  if ( type.empty() )
    return -1;
  
  if (nifty_is_name_prefix( type,  "Hyperdrive") ) return MOD_HYPERSPEED;
  if (nifty_is_name_prefix( type,  "Realspeed") ) return MOD_REALSPEED;
  if (nifty_is_name_prefix( type,  "Laser") ) return MOD_LASER;
  if (nifty_is_name_prefix( type,  "Ion") ) return MOD_ION;
  if (nifty_is_name_prefix( type,  "Shield") ) return MOD_MAXSHIELD;
  if (nifty_is_name_prefix( type,  "Fuel Pod") ) return MOD_ENERGY;
  if (nifty_is_name_prefix( type,  "Projectile Launcher") ) return MOD_LAUNCHER;
  if (nifty_is_name_prefix( type,  "Tractorbeam") ) return MOD_TRACTORBEAM;
  if (nifty_is_name_prefix( type,  "Communication System") ) return MOD_COMM;	
  if (nifty_is_name_prefix( type,  "Sensor Package") ) return MOD_SENSOR;
  if (nifty_is_name_prefix( type,  "Astronavigation System") ) return MOD_ASTRO_ARRAY;
  if (nifty_is_name_prefix( type,  "Defense System") ) return MOD_DEFENSELAUNCHER;
  if (nifty_is_name_prefix( type,  "Manuevering Jets") ) return MOD_MANUEVER;
  if (nifty_is_name_prefix( type,  "Missile Launcher") ) return MOD_MISSILE;
  if (nifty_is_name_prefix( type,  "Torpedo Launcher") ) return MOD_TORPEDO;
  if (nifty_is_name_prefix( type,  "Rocket Launcher") ) return MOD_ROCKET;
  if (nifty_is_name_prefix( type,  "Defense Launcher") ) return MOD_CHAFF;
  if (nifty_is_name_prefix( type,  "Gravity Well Projector") ) return MOD_GRAVITY_PROJ;
  if (nifty_is_name_prefix( type,  "Hull Plating") ) return MOD_HULL;
  if (nifty_is_name_prefix( type,  "Turret") ) return MOD_TURRET;
  if (nifty_is_name_prefix( type,  "Cargo") ) return MOD_CARGO;
  if (nifty_is_name_prefix( type,  "Flag") ) return MOD_FLAG;
  return -1;  	

  	// UPDATE show_mod_type2 - DV 5-15-04
}

//Use for anything but ammo - Added for cargo check ( No obj ) - DV 2/8/04
bool module_type_install2( int modtype, SHIP_DATA *ship, int modsize )
{
	MODULE_DATA *module;
//	bool onlyone = FALSE;
	int external=0, internal=0;

	for ( module = ship->first_module; module; module = module->next )
	{
		if ( ( module->type == MOD_COMM && modtype == MOD_COMM ) ||
		     ( module->type == MOD_SENSOR && modtype == MOD_SENSOR ) ||
		     ( module->type == MOD_ASTRO_ARRAY && modtype == MOD_ASTRO_ARRAY ) )
		  return TRUE;
		
		if( modtype == MOD_HULL )
		  return FALSE;

		if( module->type == MOD_HULL )
		  continue;
		
		if( is_external_mod( module->type ) )
		  external += module->size;
		
		if( is_internal_mod( module->type ) )
		  internal += module->size;
	}
		  
	if( is_external_mod( modtype ) && (external+modsize) > ship->maxextmodules)
	  return TRUE;
	if( is_internal_mod( modtype ) && (internal+modsize) > ship->maxintmodules)
	  return TRUE;
	  
	return FALSE;	  
}

bool module_type_install(OBJ_DATA *obj, SHIP_DATA *ship)
{
	MODULE_DATA *module;
//	bool onlyone = FALSE;
	int external=0, internal=0;
//	char buf[MAX_STRING_LENGTH];

//	if( obj->value[1] == MOD_COMM || obj->value[1] == MOD_ASTRO_ARRAY || obj->value[1] == MOD_SENSOR )
//	  onlyone = TRUE;

//      if( onlyone == TRUE )
	for ( module = ship->first_module; module; module = module->next )
	{
		if ( ( module->type == MOD_COMM && obj->value[1] == MOD_COMM ) ||
		     ( module->type == MOD_SENSOR && obj->value[1] == MOD_SENSOR ) ||
		     ( module->type == MOD_ASTRO_ARRAY && obj->value[1] == MOD_ASTRO_ARRAY ) )
		  return TRUE;
		
		if( obj->value[1] == MOD_HULL )
		  return FALSE;

		if( module->type == MOD_HULL )
		  continue;
		
		if( is_external_mod( module->type ) )
		  external += module->size;
		
		if( is_internal_mod( module->type ) )
		  internal += module->size;
		  
//	log_printf( "Ext: %d, Int: %d\n", external, internal );

	  
	}
	
	if( is_ammo_mod( obj->value[1] ) )
	{
	  int ammo;
	  ammo = ship->missiles + ship->torpedos*2 + ship->rockets*3;
	  if( obj->value[1] == MOD_CHAFF )
	  {
	    if( (ship->chaff + obj->value[3]) > ( ship->mod->defenselaunchers*6 ) )
	      return TRUE;
	    else
	      return FALSE;
	  }
	    
	  if( obj->value[1] == MOD_MISSILE )
	  {
	    if( ( ammo + obj->value[3] )> ( ship->mod->launchers*8 ) )
	      return TRUE;
	    else
	      return FALSE;
	  }
	  
	  if( obj->value[1] == MOD_TORPEDO )
	  {
	    if( ( ammo + (obj->value[3])*2 ) > ( ship->mod->launchers*8 ) )
	      return TRUE;
	    else
	      return FALSE;
	  }
	  
	  if( obj->value[1] == MOD_ROCKET )
	  {
	    if( ammo + (obj->value[3])*3> (ship->mod->launchers*8 ) )
	      return TRUE;
	    else
	      return FALSE;
	  }
	}

//	log_printf( buf, "Ext: %d, Int: %d\n", external, internal );
	  
	if( is_external_mod( obj->value[1] ) && (external+obj->value[2]) > ship->maxextmodules)
	  return TRUE;
	if( is_internal_mod( obj->value[1] ) && (internal+obj->value[2]) > ship->maxintmodules)
	  return TRUE;
	  
	return FALSE;
}


void do_remove_module( CHAR_DATA *ch, char *argument )
{

    std::string arg1;
    std::string arg2;
    char buf[MAX_STRING_LENGTH]; // Need to handle tokens in module names
    std::string argstr = argument;
    OBJ_DATA *obj;
    SHIP_DATA *ship = NULL;
    MODULE_DATA	*module;
    int modnum, tempnum=1, chance;


    argstr = one_argument( argstr, arg1 );
    argstr = one_argument( argstr, arg2 );

    chance = IS_NPC(ch) ? ch->top_level
             : (int) (ch->pcdata->learned[gsn_shipmaintenance]);

    if( chance <= 0 )
    {
        send_to_char( "You do not know how to install a module.\n", ch );
        return;
    }
 
    if ( arg1.empty() || !is_number( arg1 ) )
    {
        send_to_char( "Remove what?\n", ch );
        return;
    }

    if ( arg2.empty() || !str_cmp( arg2, "here" ) )
    {
      if( ( ship = ship_from_engine( ch->game, ch->in_room->vnum ) ) == NULL )
      {
          send_to_char( "Remove what from what ship?\n", ch );
          return;
      }
    }
    if ( !ship )
        ship = ship_in_room( ch->in_room , arg2 );    
    if ( !ship )            
	  {    
        act( AT_PLAIN, "I see no $T here.", ch, NULL, argstr, TO_CHAR );        
        return;       
    }

    if ( !IS_GOD(ch) && ( ship->shipclass != SHIP_DEBRIS ) && ( !check_pilot( ch , ship ) || !str_cmp( ship->owner , "Public" ) || !str_cmp( ship->owner , "Trainer" ) ) )
    {    
        send_to_char("&RHey, thats not your ship!\n",ch);    	
        return;    	
    }

    if ( number_percent( ) > chance )
    {
      send_to_char( "You fail to figure out how to install this module.\n", ch );
      return;
    }

    modnum = strtoi(arg1);

    if( modnum > 0 )
      for ( module = ship->first_module; module; module = module->next, tempnum++ )
      {

        if ( modnum == tempnum )
        {
          
          if( module->type == MOD_CARGO )
          {
            send_to_char("Use unloadcargo or transfercargo to remove cargo.\n",ch);
            return;
          }
          
          if( ship->shipclass == FIGHTER_SHIP )
            obj = create_object( get_obj_index( MOD_FIGHTER_OBJECT ), 100 );
          if( ship->shipclass == MIDSIZE_SHIP )
            obj = create_object( get_obj_index( MOD_MIDSHIP_OBJECT ), 100 );
          if( ship->shipclass == CAPITAL_SHIP )
            obj = create_object( get_obj_index( MOD_CAPSHIP_OBJECT ), 100 );
          obj->value[0] = module->condition;
          obj->value[1] = module->type;
          obj->value[2] = module->size;
          obj->value[3] = module->modification;
          SPRINTF_RUNTIME (buf, obj->description, show_mod_type( module ) );
          obj->description =  STRALLOC( buf );
          SPRINTF (buf, "%s", module->name);
          obj->short_descr =  STRALLOC( buf );
          obj->name =  STRALLOC( buf );
          
          act( AT_ACTION, "$n removes $p.", ch, obj, NULL, TO_ROOM );	
          act( AT_ACTION, "You remove $p.", ch, obj, NULL, TO_CHAR );
          obj = obj_to_char( obj, ch );
          
          if ( module->type == MOD_FLAG )
          {
            ROOM_INDEX_DATA *froom;
            if ( (froom = get_room_index(module->condition)) != NULL )
              BV_REMOVE_BIT( froom->room_flags, modflags[module->modification] );
          }
          
          UNLINK( module, ship->first_module, ship->last_module, next, prev );

          STRFREE(module->name);
          DISPOSE( module );

          ship->modules = ship->modules -1;

          update_ship_modules( ship );
                updateship( ship, obj->value[1]);
          save_ship(ship);
          return;
        }
      }
	  send_to_char("No such module installed\n",ch);
	  return;
}

void do_show_modules( CHAR_DATA *ch, char *argument )
{

    std::string arg;
    std::string argstr = argument;
    SHIP_DATA *ship = NULL;
    MODULE_DATA *module;
    std::string buf;
    int modcounter = 1, modtype = -1;
    bool shipsearch = FALSE;

    argstr = one_argument( argstr, arg );

    if ( !arg.empty() )
    {
	      ship = ship_in_room( ch->in_room , arg );    

      /*if ( !ship )            
	      {    
            act( AT_PLAIN, "I see no ship here.", ch, NULL, argstr, TO_CHAR );        
            return;       
        }
      */        
        if ( ship && ( ship->shipclass != SHIP_DEBRIS ) && !check_pilot( ch , ship ) && !IS_GOD(ch) ) 
        {    
          send_to_char("&RHey, thats not your ship!\n",ch);    	
          return;    	
        }
    }
    
    if ( ship )
        shipsearch = TRUE;
    if ( !ship )
        ship = ship_from_cockpit( ch->game, ch->in_room->vnum );
        
    if( !ship )
    {
        send_to_char( "Show the modules on what ship?\n", ch );
        return;
    }
	
    modtype = shipsearch ? (show_mod_type3( argstr )) : (show_mod_type3( arg ) );
    
    buf = str_printf("Modules installed on %s:\n\n", ship->name );
    send_to_char(buf,ch);
    for ( module = ship->first_module; module; modcounter++, module = module->next )
    {
      if ( modtype != -1 )
        if ( module->type != modtype )
          continue;
      buf = str_printf("%d) Name: %s\n\tType: %s Condition: %d Size: %d Mod: %d\n", 
              modcounter, module->name, show_mod_type( module ), module->condition, module->size, module->modification );
      send_to_char(buf,ch);
    }
    if (!ship->modules)
    {
      send_to_char( "No Modules installed.\n",ch);
      return;
    }
    ch_printf(ch,"Installed: External %d/%d  Internal %d/%d.\n", 
              get_extmodule_count(ship), ship->maxextmodules, get_intmodule_count(ship), ship->maxintmodules );
    
	
	return;
}



void do_load( CHAR_DATA *ch, char *argument )
{
    SHIP_DATA *ship;
    std::string arg1;
    std::string arg2;
    std::string arg3;
    std::string argstr = argument;
    int amountCargo;
    int typeCargo;

    argstr = one_argument( argstr , arg1);
    argstr = one_argument( argstr , arg2);
    argstr = one_argument( argstr , arg3);

    if ( arg1.empty() || arg2.empty() )
    {
      send_to_char( "Syntax: load <cargotype> <amount> [ship]", ch);
      return;
    }

    amountCargo = strtoi(arg2);
    typeCargo = strtoi(arg1);
    
    if (  (ship = ship_from_cockpit(ch->game, ch->in_room->vnum))  == NULL )
    {
        if ( arg3.empty() )
        {
            act( AT_PLAIN, "Which ship do you want to load?.", ch, NULL, NULL, TO_CHAR );
            return;
        }

        ship = ship_in_room( ch->in_room , arg3 );
        if ( !ship )
        {
            act( AT_PLAIN, "I see no $T here.", ch, NULL, arg3, TO_CHAR );
            return;
        }

      //target = ship;
    }
    else if ( ship->hanger == ch->in_room->vnum )
    {
        if ( arg3.empty() )
        {
            act( AT_PLAIN, "Which ship do you want to load?.", ch, NULL, NULL, TO_CHAR );
            return;
        }

        ship = ship_in_room( ch->in_room , arg3 );
        if ( !ship )
        {
            act( AT_PLAIN, "I see no $T here.", ch, NULL, arg3, TO_CHAR );
            return;
        }

        //target = ship;
    }

    else
      //target = ship;
    
    if (!typeCargo)
    {
      switch(UPPER(arg1[0]))
      {
      	case 'e':
      	  typeCargo = 3;
      	  break;
      	case 'f':
      	  typeCargo = 1;
      	  break;
      	case 'l':
      	  typeCargo = 4;
      	  break;
      	case 'm':
      	  if( !str_cmp(arg1, "metal") )
      	    typeCargo = 2;
      	  else if( !str_cmp(arg1, "medical") )
      	    typeCargo = 5;
      	  break;
      	case 's':
      	  typeCargo = 6;
      	  break;
      	case 'v':
      	  typeCargo = 8;
      	  break;
      	case 'w':
      	  typeCargo = 7;
      	  break;
      	default:
          send_to_char( "That cargo type is not recognized.", ch);
          return;      	  
          break;
      }
    }

//  if ( ship->maxcargo < totalcargo + amountCargo )
    {
           send_to_char( "That ship can not fit this much cargo.", ch);
           return;      	  
    }
         
    
    affectshipcargo( ship, typeCargo, amountCargo );
    
    send_to_char( "Cargo loaded.", ch);
    

}

void affectshipcargo( SHIP_DATA *ship, int typeCargo, int amount )
{
  switch(typeCargo)
  {
/*  case 0:
      ship->cargo0 += amount;
      break;
    case 1:
      ship->cargo1 += amount;
      break;
    case 2:
      ship->cargo2 += amount;
      break;
    case 3:
      ship->cargo3 += amount;
      break;
    case 4:
      ship->cargo4 += amount;
      break;
    case 5:
      ship->cargo5 += amount;
      break;
    case 6:
      ship->cargo6 += amount;
      break;
    case 7:
      ship->cargo7 += amount;
      break;
    case 8:
      ship->cargo8 += amount;
      break;
    case 9:
      ship->cargo9 += amount;
      break;
*/
    default: break;
  }
  return;
}

void do_unload( CHAR_DATA *ch, char *argument )
{
    send_to_char( "This is not implemented yet!\n", ch);
    return;
}

void do_upgradeship( CHAR_DATA *ch, char *argument )
{
	return;
}

void do_degradeship( CHAR_DATA *ch, char *argument )
{
	return;
}

void do_gravityprojector(CHAR_DATA * ch, char *argument)
{

    std::string arg;
    int gravpower = 0;
    SHIP_DATA *ship;
    std::string buf;
    arg= argument;

    if(arg.empty())
    {
        send_to_char("Syntax: gravityprojector <on/off/amt>\n",ch);
        return;
    }
    
    if ( (ship = ship_from_cockpit(ch->game, ch->in_room->vnum)) == NULL )
    {
        send_to_char("&RYou must be in the cockpit of a ship to do that!\n",ch);
        return;
    }

    if ( ship->shipclass > SHIP_PLATFORM )
    {
        send_to_char("&RThis isn't a spacecraft!\n",ch);
        return;
    }


    if (autofly (ship))
    {
        send_to_char("The security system will not allow you to activate a gravity well with the autopilot online.\n" , ch );
        return;
    }

    if ( ship->mod->gravitypower == 0 )
    {
        send_to_char("There are no gravity well projectors installed in this craft.\n" , ch );
        return;
    }

    if ( (ship = ship_from_coseat(ch->game, ch->in_room->vnum)) == NULL )
    {
        send_to_char("&RYou need to be in the pilot seat!\n",ch);
        return;
    }

    if (ship->shipstate == SHIP_DISABLED)
    {
        send_to_char("&RThe ships drive is disabled. No power available.\n",ch);
        return;
    }

    if (ship->shipstate == SHIP_LANDED)
    {
        send_to_char("&ROne needs to take off first!\n",ch);
        return;
    }


    if (ship->shipstate == SHIP_HYPERSPACE)
    {
        send_to_char("&RYou can only do that in realspace!\n",ch);
        return;
    }

    if (ship->shipstate != SHIP_READY)
    {
        send_to_char("&RPlease wait until the ship has finished its current manouver.\n",ch);
        return;
    }

    buf = "Your sensors ring an alarm as a " + std::string(ship->name) + " brings up its gravity well.";

    if (nifty_is_name_prefix (arg, "on"))
    {
        ship->mod->gravproj = ship->mod->gravitypower;
        send_to_char("You activate the gravity projectors at full power.\n",ch);
        echo_to_system (AT_PLAIN, ship, buf, NULL);
        return;
    }    

    buf = "Your sensors ring an alarm as a " + std::string(ship->name) + " disengages its gravity well.";

    if (nifty_is_name_prefix (arg, "off"))
    {
        ship->mod->gravproj = 0;
        send_to_char("You deactivate the gravity projectors.\n",ch);
        echo_to_system (AT_PLAIN, ship, buf, NULL);
        return;
    }    

    if ( is_number( arg ) )
    {
        gravpower = strtoi(arg);
        ship->mod->gravproj = UMIN(gravpower, ship->mod->gravitypower);
        if( ship->mod->gravproj < ship->mod->gravitypower )
          send_to_char("You activate the gravity projectors at partial power.\n",ch);
        else
          send_to_char("You activate the gravity projectors at full power.\n",ch);
        echo_to_system (AT_PLAIN, ship, buf, NULL);
        return;
    }

    return;

}

int get_template_price( int templatetype )
{
    int price = 0;
    int shipclass = 0;
    
    price += (templatetypes[templatetype].maxextmodules)*400;
    price += (templatetypes[templatetype].maxintmodules)*200;
    price += (templatetypes[templatetype].weight)*4;

    shipclass = ((templatetypes[templatetype].shipclass%10));
    
    if ( shipclass == CAPITAL_SHIP )
      price *= 10;

    return price;
}

char * get_template_string( int templatetype )
{
  //char buf[MAX_STRING_LENGTH];
    char *templatestring;
    int i;

    for ( i = 0; i < MAX_TEMPLATETYPE; i++ )
      if( templatetypes[i].type == templatetype )
      {
        templatestring = STRALLOC( templatetypes[i].string );
        break;
      }
    
    if( i >= MAX_TEMPLATETYPE ) 
      templatestring = STRALLOC( "" );

  /*
    switch(templatetype)
    {
      case 1: 
  //  	  SPRINTF( buf, "2] 1; 2)20:1" );
        SPRINTF( buf, "2] 1)22:2" );
        templatestring = STRALLOC( buf ); 
        break;
      case 2:
        SPRINTF( buf, "1] 1; " );
        templatestring = STRALLOC( buf );
        break;
      case 3:
        SPRINTF( buf, "1] 10;" );
        templatestring = STRALLOC( buf );
        break;
      default: send_to_char( "Template not found.\n", ch );return; break;
    }
  */

    return templatestring;
}

void do_maketemplateship(CHAR_DATA * ch, char *argument)
{
    SHIP_DATA *ship;
    SHIP_MOD_DATA *ship_mod;
    std::string buf;
    char *templatestring;
    std::string arg;
    std::string arg2;
    std::string argstr = argument;
    int templatetype, i;
    
    argstr = one_argument( argstr, arg );
    argstr = one_argument( argstr, arg2 );

    if ( argstr.empty() )
    {
        send_to_char( "Usage: maketemplateship <templatetype> <filename> <newshipname>\n", ch );
        return;
    }
    
    if( !is_number( arg ) )
    {
        send_to_char( "Template type must be a number.\n", ch );
        return;
    }
  
    if( arg2.empty() || argstr.empty() )
    {
        send_to_char( "Usage: maketemplateship <templatetype> <filename> <newshipname>\n", ch );
        return;
    }

    if ( !ch || !ch->game )
    {
        send_to_char( "You need to be a game to make a ship template.\n", ch );
        return;
    }

    templatetype = strtoi(arg);
    
    templatestring = get_template_string( templatetype );
    
    if ( !templatestring || templatestring[0] == '\0' )
    {
      for ( i = 0; i < MAX_TEMPLATETYPE; i++ )
      {
          ch_printf( ch, "Num: %d Name: %s MaxExt: %d MaxInt: %d\n   Desc: %50s\n",
              templatetypes[i].type, templatetypes[i].name, 
              templatetypes[i].maxextmodules, templatetypes[i].maxintmodules,
              templatetypes[i].desc );
      }
      return;
    }
    CREATE( ship, SHIP_DATA, 1 );
    ship->game = ch->game;

    LINK( ship, first_ship, last_ship, next, prev );

    ship->owner         = STRALLOC ("");
    ship->copilot       = STRALLOC ("");
    ship->pilot         = STRALLOC ("");
    ship->home          = STRALLOC ("");
    ship->description   = STRALLOC ("");

    buf = templatestring;
    ship->templatestring = STRALLOC(buf);

    ship->type          = SHIP_CIVILIAN;
    ship->shipclass         = FIGHTER_SHIP;
    ship->lasers        = 0;
    ship->missiles      = 0;
    ship->rockets       = 0;
    ship->torpedos      = 0;
    ship->maxshield     = 0;
    ship->maxhull       = 0;
    ship->maxenergy     = 0;
    ship->hyperspeed    = 0;
    ship->chaff         = 0;
    ship->realspeed     = 0;
    ship->currspeed     = 0;
    ship->manuever      = 0;

    ship->shipstate     = SHIP_LANDED;
    ship->docking       = SHIP_READY;
    ship->statei0       = LASER_READY;
    ship->statet0       = LASER_READY;
    ship->statettractor = SHIP_READY;
    ship->statetdocking = SHIP_READY;
    ship->missilestate  = MISSILE_READY;

    ship->spaceobject   = NULL;
    ship->energy        = 0;
    ship->hull          = 1;
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
    
    ship->shipID = ch->game->get_sysdata()->get_and_increment_shipID();
    save_sysdata(*ch->game->get_sysdata());

    ship->name          = STRALLOC (templatetypes[templatetype].name);
    ship->owner         = STRALLOC ("");
    ship->filename         = STRALLOC (arg2);
    buf = "TemplateShip " + std::to_string(ship->shipID) + " " + argstr;
    ship->personalname  = STRALLOC (buf);
    
    transship( ship, 45 );

    if ( parse_ship_template(templatestring, ship) )
      {
        bug( "maketemplateship: parse_ship_template failed.\n", 0 );
        shipdelete(ship, FALSE);
        return;
      }
    
    save_ship( ship );
    write_ship_list( ch->game );

    return;	
}


void do_ordership( CHAR_DATA *ch, char *argument )
{
    SHIP_DATA *ship;
    SHIP_MOD_DATA *ship_mod;
    std::string buf;
    std::string argstr = argument;
    char *templatestring;
    std::string arg;
    int templatetype, i, shipprice, shipclass, shiptype;
    
    argstr = one_argument( argstr, arg );

    if ( ch->in_room->vnum != 5993 && ch->in_room->vnum != 32000 && ch->in_room->vnum != 1900 && ch->in_room->vnum != 1102 && ch->in_room->vnum != 100 && ch->in_room->vnum != 32050 )
    {
        send_to_char( "This is not one of the operational shipyards.\n", ch );
        return;
    }
    
    if( !is_number( arg ) )
    {
        send_to_char( "Template type must be a number.\n", ch );
        for ( i = 0; i < MAX_TEMPLATETYPE; i++ )
        {
            ch_printf( ch, "Num: %d Name: %s MaxExt: %d MaxInt: %d Price: %d\n   Desc: %50s\n",
                templatetypes[i].type, templatetypes[i].name, 
                templatetypes[i].maxextmodules, templatetypes[i].maxintmodules,
                get_template_price(i),
                templatetypes[i].desc );
        }
        return;
    }
  
    if ( argstr.empty() )
    {
        send_to_char( "Usage: ordership <templatetype> <newshipname>\n", ch );
        return;
    }
    
    templatetype = strtoi(arg);
    
    templatestring = get_template_string( templatetype );
    
    if ( !templatestring || templatestring[0] == '\0' )
    {
      for ( i = 0; i < MAX_TEMPLATETYPE; i++ )
      {
          ch_printf( ch, "Num: %d Name: %s MaxExt: %d MaxInt: %d Price: %d\n   Desc: %50s\n",
              templatetypes[i].type, templatetypes[i].name, 
              templatetypes[i].maxextmodules, templatetypes[i].maxintmodules,
              get_template_price(i),
              templatetypes[i].desc );
      }
      return;
    }

    shipclass = ((templatetypes[templatetype].shipclass%10));
    shiptype = ((int)templatetypes[templatetype].shipclass/10);

    if( shiptype == 2 )
      if ( ch->in_room->vnum != 32000 )
      {
        send_to_char( "This ship can only be built on an imperial shipyard.\n", ch );
        return;
      }

    if( shiptype == 1 )
      if ( ch->in_room->vnum != 1900 )
      {
        send_to_char( "This ship can only be built on an rebel shipyard.\n", ch );
        return;
      }
    
    shipprice = get_template_price(templatetype);

    if ( ch->gold < shipprice )
    {
        ch_printf(ch, "&RYou don't have enough credits to purchase this ship.\n");
        return;
    }

    if ( get_ship( ch->game, argstr ) )
    {
        ch_printf(ch, "&RA ship already exists by that name.\n");
        return;
    }
    
    if ( argstr.find('&') != std::string::npos )
    {            
        send_to_char( "&RColors may not be used in a ship's name.\n", ch);
        return;
    }

    if ( !ch || !ch->game )
    {
        send_to_char( "You need to be a game to order a ship.\n", ch );
        return;
    }

    ch->gold -= shipprice;
    
    CREATE( ship, SHIP_DATA, 1 );
    ship->game = ch->game;

    LINK( ship, first_ship, last_ship, next, prev );

    ship->copilot       = STRALLOC ("");
    ship->pilot         = STRALLOC ("");
    ship->home          = STRALLOC ("");
    ship->description   = STRALLOC ("");

    buf = templatestring;
    ship->templatestring = STRALLOC(buf);

    switch( shipclass )
    {
        case 0: ship->shipclass = FIGHTER_SHIP; break;
        case 1: ship->shipclass = MIDSIZE_SHIP; break;
        case 2: ship->shipclass = CAPITAL_SHIP; break;
        default: ship->shipclass = FIGHTER_SHIP; break;
    }
    switch( shiptype )
    {
        case 0: ship->type = SHIP_CIVILIAN; break;
        case 1: ship->type = SHIP_REBEL; break;
        case 2: ship->type = SHIP_IMPERIAL; break;
        default: ship->type = SHIP_CIVILIAN; break;
    }

    ship->lasers        = 0;
    ship->missiles      = 0;
    ship->rockets       = 0;
    ship->torpedos      = 0;
    ship->maxshield     = 0;
    ship->maxhull       = 0;
    ship->maxenergy     = 0;
    ship->hyperspeed    = 0;
    ship->chaff         = 0;
    ship->realspeed     = 0;
    ship->currspeed     = 0;
    ship->manuever      = 0;

    ship->shipstate     = SHIP_LANDED;
    ship->docking       = SHIP_READY;
    ship->statei0       = LASER_READY;
    ship->statet0       = LASER_READY;
    ship->statettractor = SHIP_READY;
    ship->statetdocking = SHIP_READY;
    ship->missilestate  = MISSILE_READY;

    ship->spaceobject   = NULL;
    ship->energy        = 0;
    ship->hull          = 1;
    ship->currjump      = NULL;
    ship->target0       = NULL;
    ship->tractoredby   = NULL;
    ship->tractored     = NULL;
    ship->docked        = NULL;
    ship->autopilot     = FALSE;
    
    CREATE( ship_mod, SHIP_MOD_DATA, 1 );
    ship->mod = ship_mod;
    update_ship_modules(ship);
    
    ship->shipID = ch->game->get_sysdata()->get_and_increment_shipID();
    save_sysdata(*ch->game->get_sysdata());

    ship->name          = STRALLOC (templatetypes[templatetype].name);
    buf = "template" + std::to_string(ship->shipID) + ".ship";
    ship->filename         = STRALLOC (buf);
    ship->personalname  = STRALLOC (argstr);
    ship->owner         = STRALLOC (ch->name);

    ship->maxextmodules = templatetypes[templatetype].maxextmodules;
    ship->maxintmodules = templatetypes[templatetype].maxintmodules;
    ship->weight	      = templatetypes[templatetype].weight;
    
    transship( ship, ch->in_room->vnum );

    if ( parse_ship_template(templatestring, ship) )
      {
        bug( "maketemplateship: parse_ship_template failed.\n", 0 );
        shipdelete(ship, FALSE);
        return;
      }
    
    save_ship( ship );
    write_ship_list( ch->game);
    
    return;  
}

void do_shipdelete( CHAR_DATA *ch, char *argument )
{
    SHIP_DATA *ship;
    
    if ( ( ship = get_ship( ch->game, argument ) ) == NULL )
    {
        ch_printf(ch, "&RNo ship exists with that name.\n");
        return;
    }
    
    shipdelete( ship, TRUE );
    return;
}

void do_transferownership( CHAR_DATA *ch, char *argument )
{
    SHIP_DATA *ship;
    std::string arg;
    std::string argstr = argument;
      
    if ( argstr.empty() )
    {
        ch_printf(ch, "&RSyntax: transferownership '<new owner>' <ship name>.\n");
        return;
    }
    
    argstr = one_argument( argstr, arg );
    
    if ( arg.empty() || argstr.empty() )
    {
        ch_printf(ch, "&RSyntax: transferownership <new owner> <ship name>.\n");
        return;
    }
    
    if ( ( ship = get_ship( ch->game, argstr ) ) == NULL )
    {
        ch_printf(ch, "&RNo ship exists with that name.\n");
        return;
    }
    
    if ( !ship->owner || ship->owner[0] == '\0' )
    {
        ch_printf(ch, "&RYou must be the owner of this ship to transfer ownership.\n");
        return;
    }
    
    
    if ( strcmp(ship->owner, ch->name ) )
    {
        ch_printf(ch, "&RYou must be the owner of this ship to transfer ownership.\n");
        return;
    }
    
  //  arg1 = strlower( arg ).c_str());
    arg[0] = UPPER(arg[0]);
    STRFREE( ship->owner );
    ship->owner = STRALLOC( arg );

    save_ship( ship );

    ch_printf(ch, "&ROwnership transferred.\n");
    
    log_printf( "%s transferred ship ownership to %s.\n", ch->name, arg );
    
    return;
}

bool add_random_modules( SHIP_DATA *ship, SHIP_DATA *origship )
{
    MODULE_DATA *module;
    MODULE_DATA *origmodule;
    MODULE_DATA *module_next;
    int number;
    int ranmod;
    int i, j;

    if ( !ship || !origship || !origship->first_module )
        return FALSE;
      
    number = number_range( 1, 3 );

    for ( j = 0; j < number; j++ )
    {
        ranmod = number_range( 1, 100 );

        for ( origmodule = origship->first_module, i = 0; i < ranmod; i++)
        {
          module_next = origmodule->next;
          origmodule = module_next;
          if ( origmodule == NULL )
            origmodule = origship->first_module;
        }
            
        ship->modules += 1;
      
        CREATE( module, MODULE_DATA, 1 );
      
        module->name =  STRALLOC( origmodule->name );
        module->type = origmodule->type;
        module->condition = origmodule->condition;
        module->size = origmodule->size;
        module->modification = origmodule->modification;
      
        LINK( module, ship->first_module, ship->last_module, next, prev );

        update_ship_modules( ship );
        updateship( ship, module->type );
      
    }

    return TRUE;
}

const std::string& get_cargo_name( int cargotype )
{
    static std::string cargoname;
    cargoname = get_flag_name(cargo_names, cargotype, MAX_CARGO_NAMES);
    return cargoname;
}

bool check_cargo( SHIP_DATA *ship, int cargotype, int nummod )
{
    MODULE_DATA *module = NULL;
    int shipnummod = 0;

    if ( !ship->first_module )
      return FALSE;

    for ( module = ship->first_module; module; module = module->next )
    {
      if ( module->modification == cargotype )
        shipnummod++;
    }

    if ( nummod <= shipnummod )
      return TRUE;
      
    return FALSE;
}

void remove_cargo( SHIP_DATA *ship, int cargotype, int nummod )
{
    MODULE_DATA *module = NULL;
    MODULE_DATA *module_next = NULL;
    int shipnummod = 0;
  //char buf[MAX_STRING_LENGTH];
    
    if ( !check_cargo( ship, cargotype, nummod ) )
    {
      log_printf( "remove_cargo: Ship %s failed check_cargo check\n", ship->personalname );
      return;
    }

    for ( module = ship->first_module; ( module && shipnummod < nummod ); module = module_next )
    {
      module_next = module->next;
      if ( module->modification == cargotype )
      {
        UNLINK( module, ship->first_module, ship->last_module, next, prev );

        STRFREE(module->name);
        DISPOSE( module );

        ship->modules = ship->modules -1;
        shipnummod++;
      }
    }
    
    update_ship_modules( ship );
    save_ship(ship);

    return;
}

void add_cargo( SHIP_DATA *ship, int cargotype, int nummod )
{
    MODULE_DATA *module;
    std::string cargoname;
    int i;
    
    cargoname = get_cargo_name(cargotype);

    for ( i = 0; i<nummod; i++ )
    {  
      ship->modules += 1;
    
      CREATE( module, MODULE_DATA, 1 );
    
      module->name =  STRALLOC( cargoname );
      module->type = MOD_CARGO;
      module->condition = 100;
      module->size = 1;
      module->modification = cargotype;
    
      LINK( module, ship->first_module, ship->last_module, next, prev );

      update_ship_modules( ship );
      save_ship(ship);
    }

    return;
}

void do_loadcargo( CHAR_DATA *ch, char *argument )
{
    
    SHIP_DATA *ship = NULL;
    int nummod, cargotype, cargoprice;
    std::string arg1;
    std::string argstr = argument;
    SPACE_DATA *spaceobject = NULL;
    CARGO_DATA_LIST *cargolist;


    argstr = one_argument( argstr, arg1 );

    if ( !is_number( arg1 ) || argstr.empty() || !is_number( argstr.c_str() ) )
    {
        send_to_char( "Syntax: loadcargo <type #> <amount>\n", ch );
        return;
    }
    
    cargotype = strtoi(arg1);
    nummod = strtoi(argstr);
    
    if ( ( ship = ship_from_cockpit( ch->game, ch->in_room->vnum ) ) == NULL )
    {
        send_to_char( "You must be in a ship's cockpit for that!\n", ch );
        return;
    }

    if ( module_type_install2(MOD_CARGO, ship, nummod) )
    {
        send_to_char( "There is no room for that much more cargo!\n", ch);
        return;
    }

    if ( cargotype > MAX_CARGO_NAMES )
    {
        send_to_char( "No such cargo!\n", ch );
        return;
    }
    
    spaceobject = spaceobject_from_vnum( ship->game, ship->location );
    
    if( !spaceobject )
    {
        send_to_char( "You need to be on a planet landing pad to buy cargo!\n", ch);
        return;
    }
    
    if( !spaceobject->first_cargo )
    {
        send_to_char( "There is no cargo available here.\n", ch);
        return;
    }

    for( cargolist = spaceobject->first_cargo; cargolist; cargolist = cargolist->next )
    {
        if ( cargolist->cargo->cargotype == cargotype )
          break;
    }
    
    if ( !cargolist )
    {
        send_to_char( "That type of cargo is not available here.\n", ch);
        return;
    }
    
    cargoprice = cargolist->cargo->price;
    
    if ( nummod <= 0 )
    {
        send_to_char( "A positive <amount> needs to be entered.\n", ch );
        return;
    }

    if ( ch->gold < nummod*cargoprice )
    {
        ch_printf( ch, "You need %d credits to purchase %d units of this cargo.\n", nummod*cargoprice, nummod );
        return;
    }
    
  /*SPRINTF( cargoname, get_cargo_name(cargotype) );

    for ( i = 0; i<nummod; i++ )
    {  
      ship->modules += 1;
    
      CREATE( module, MODULE_DATA, 1 );
    
      module->name =  STRALLOC( cargoname );
      module->type = MOD_CARGO;
      module->condition = 100;
      module->size = 1;
      module->modification = cargotype;
    
      LINK( module, ship->first_module, ship->last_module, next, prev );

      update_ship_modules( ship );
      save_ship(ship);

    }
  */

    add_cargo( ship, cargotype, nummod );
    
    send_to_char( "Your displays show your cargo request has been delivered.\n", ch );
    
    ch->gold -= nummod*cargoprice;
    
    return;
}

void do_checkcargo( CHAR_DATA *ch, char *argument )
{
    SHIP_DATA *ship = NULL;
    SPACE_DATA *spaceobject = NULL;
    CARGO_DATA_LIST *cargolist;
    int defprice;
    if ( ( ship = ship_from_cockpit( ch->game, ch->in_room->vnum ) ) == NULL )
    {
      send_to_char( "You must be in a ship's cockpit to access that information.\n", ch );
      return;
    }

    spaceobject = spaceobject_from_vnum( ship->game, ship->location );
    
    if( !spaceobject )
    {
      send_to_char( "You need to be on a planet landing pad to buy cargo!\n", ch);
      return;
    }
    
    if( !spaceobject->first_cargo )
    {
      send_to_char( "There is no cargo available here.\n", ch);
      return;
    }
    else
    {
      send_to_char( "\nCargo name    Cargo type#  Price   (compare).\n", ch);
    }
    


    for( cargolist = spaceobject->first_cargo; cargolist; cargolist = cargolist->next )
    {
      if ( cargolist->cargo )
      {
        defprice = cargodefaults[cargolist->cargo->cargotype].price;
        ch_printf( ch, "%12s  %11d  %5d   (%s)\n", get_cargo_name(cargolist->cargo->cargotype).c_str(), 
        cargolist->cargo->cargotype, cargolist->cargo->price, 
        (cargolist->cargo->price > defprice + (int) sqrt(defprice)/4 ? "High" :
        (cargolist->cargo->price < defprice - (int) sqrt(defprice)/4 ? "Low" : "Medium" )));
      }
    }

    return;
}

void do_transfercargo( CHAR_DATA *ch, char *argument )
{
    SHIP_DATA *ship = NULL;
    SHIP_DATA *eShip = NULL;
    int nummod, cargotype;
    std::string arg1;
    std::string arg2;
    std::string argstr = argument;
    std::string cargoname;

    argstr = one_argument( argstr, arg1 );
    argstr = one_argument( argstr, arg2 );

    if ( arg1.empty() || !is_number( arg1 ) || arg2.empty() || !is_number( arg2 ) )
    {
        send_to_char( "Syntax: transfercargo <type #> <amount> <ship>\n", ch );
        return;
    }

    cargotype = strtoi(arg1);
    nummod = strtoi(arg2);
    
    if ( ( ship = ship_from_cockpit( ch->game, ch->in_room->vnum ) ) == NULL )
    {
        send_to_char( "You must be in your ship's cockpit for that!\n", ch );
        return;
    }
    
    if ( !check_pilot( ch , ship ) )
      {
        send_to_char("&RYou need to have pilot status to transfer cargo on this ship.\n",ch);
        return;
      }
    
    if ( cargotype > MAX_CARGO_NAMES )
    {
        send_to_char( "No such cargo!\n", ch );
        return;
    }

    if ( nummod <= 0 )
    {
        send_to_char( "A positive <amount> needs to be entered.\n", ch );
        return;
    }

    if ( !check_cargo( ship, cargotype, nummod ) )
    {
        cargoname = get_cargo_name(cargotype);  
        ch_printf( ch, "An error is displayed: Not that much cargo of %s.\n", cargoname.c_str() );
        return;
    }
    
    eShip = ship_from_hanger( ship->game, ship->location );

    if ( !eShip )
    {
        eShip = get_ship( ship->game, argstr );

        if( eShip && eShip->docked != ship )
          eShip = NULL;
    }

    if ( !eShip )
        eShip = ship->docked;

    if ( eShip == ship )
        eShip = NULL;
      
  //ch_printf( ch, "Ship: %d, eShip: %d\n", (int) ship, (int) eShip );  
      
    if ( !eShip )
    {
        act( AT_PLAIN, "An error is displayed: No ship to transfer to was found.", ch, NULL, argstr, TO_CHAR );
        return;
    }

    if ( module_type_install2(MOD_CARGO, eShip, nummod) )
    {
        send_to_char( "There is no room on the destination ship for that much more cargo!\n", ch);
        return;
    }

  //ch_printf( ch, "Ship: %d eShip: %d Argument: '%s'.\n", (int) ship, (int) eShip, argument );
    
    remove_cargo( ship, cargotype, nummod );
    add_cargo( eShip, cargotype, nummod );
    
    send_to_char( "Your display shows your cargo has been transferred.\n", ch );
    
    
}
void do_unloadcargo( CHAR_DATA *ch, char *argument )
{
  
    SHIP_DATA *ship = NULL;
    int nummod, cargotype, cargoprice;
    std::string arg1;
    std::string argstr = argument;
    std::string cargoname;
    SPACE_DATA *spaceobject = NULL;
    CARGO_DATA_LIST *cargolist;


    argstr = one_argument( argstr, arg1 );

    if ( !is_number( arg1 ) || argstr.empty() || !is_number( argstr ) )
    {
        send_to_char( "Syntax: unloadcargo <type #> <amount>\n", ch );
        return;
    }
    
    cargotype = strtoi(arg1);
    nummod = strtoi(argstr);
    
    if ( ( ship = ship_from_cockpit( ch->game, ch->in_room->vnum ) ) == NULL )
    {
        send_to_char( "You must be in a ship's cockpit for that!\n", ch );
        return;
    }
    
    if ( !check_pilot( ch , ship ) )
      {
          send_to_char("&RYou need to have pilot status to unload cargo on this ship.\n",ch);
          return;
      }
    
    if ( cargotype > MAX_CARGO_NAMES )
    {
        send_to_char( "No such cargo!\n", ch );
        return;
    }
    
    spaceobject = spaceobject_from_vnum( ship->game, ship->location );
    
    if( !spaceobject )
    {
        send_to_char( "You need to be on a planet landing pad to sell cargo!\n", ch);
        return;
    }
    
    if( !spaceobject->first_cargo )
    {
        send_to_char( "You can not sell cargo here.\n", ch);
        return;
    }

    for( cargolist = spaceobject->first_cargo; cargolist; cargolist = cargolist->next )
    {
        if ( cargolist->cargo->cargotype == cargotype )
          break;
    }
    
    if ( !cargolist )
    {
        send_to_char( "That type of cargo is not available to be sold here.\n", ch);
        return;
    }
    
    cargoprice = cargolist->cargo->price;
    
    if ( nummod <= 0 )
    {
        send_to_char( "A positive <amount> needs to be entered.\n", ch );
        return;
    }

    if ( !check_cargo( ship, cargotype, nummod ) )
    {
        cargoname = get_cargo_name(cargotype);  
        ch_printf( ch, "An error is displayed: Not that much cargo of %s.\n", cargoname.c_str() );
        return;
    }
    
    remove_cargo( ship, cargotype, nummod );
    
    send_to_char( "Your displays show your cargo request has been delivered.\n", ch );
    
    ch->gold += nummod*cargoprice;
    
    return;
}

void do_restoreship( CHAR_DATA *ch, char *argument )
{
  //SHIP_DATA *ship;
    std::string buf;
    std::string buf2;
  //struct stat fst;
    
    buf = str_printf( "%s%s", BACKUPSHIP_DIR, argument );
    buf2 = str_printf( "%s%s", SHIP_DIR, argument );
    
  /*if ( !stat( buf, &fst ) )
    {
      send_to_char( "File not found.\n", ch );
      return;
    }
  */  
    rename( buf.c_str(), buf2.c_str() );

    if (!load_ship_file( ch->game,argument ) )
    {
      send_to_char( "Error loading file.\n", ch );
      return;
    }

    write_ship_list( ch->game );
    
    return;
}

sh_int get_acceleration( SHIP_DATA *ship )
{
    int accel = 0;
    int speedmod;
    
    accel += ship->mod->maxhull/5;
    accel += ship->weight/10;
    accel += ship->energy/40;
    
    speedmod = 5000 + (ship->mod->realspeed/3);
    
    if ( accel )
      accel = speedmod/accel;
      
    if( ship->shipclass == CAPITAL_SHIP )
    {
      accel *= 3;
      if ( !accel )
        accel = 1;
    }
    
    return accel;
}

bool set_random_cargo( SPACE_DATA *spaceobject, CARGO_DATA_LIST *cargolist )
{
    int defprice, calcrange;
    CARGO_DATA_LIST *cargoscroll;
    bool dup;  // Variable for checks for duplicates of cargo type in spaceobject

    if ( !cargolist )
      return FALSE;
    //Randomize cargo - DV 3-15-04

    do // Checks for duplicates of cargo type in spaceobject 
          // ( Come up with better way - DV 3-16-04 )
    {
      cargolist->cargo->cargotype = number_range( 0, CARGOTYPE_DEFAULT-1 );
      
      dup = FALSE;

      for ( cargoscroll = spaceobject->first_cargo; cargoscroll && dup == FALSE; cargoscroll = cargoscroll->next )
      {
        if( cargolist != cargoscroll )
          dup = ( cargolist->cargo->cargotype == cargoscroll->cargo->cargotype );
      }
      
    } while ( dup == TRUE );

    defprice = cargodefaults[cargolist->cargo->cargotype].price;
    
    calcrange = ((int) sqrt(defprice))/2;

    cargolist->cargo->price = defprice + number_range( -1*defprice/calcrange, defprice/calcrange );

    return TRUE;

}

bool add_random_cargo( SPACE_DATA *spaceobject )
{
    CARGO_DATA_LIST *cargolist, *cargoscroll;
    CARGO_DATA *cargo;
    bool dup = FALSE;  // Variable for checks for duplicates of cargo type in spaceobject

    int defprice, calcrange;

    if ( !spaceobject )
      return FALSE;
    //Add a random cargo amount onto a spaceobject - DV 3-15-04
    CREATE( cargolist, CARGO_DATA_LIST, 1 );
    
    LINK( cargolist, spaceobject->first_cargo, spaceobject->last_cargo, next, prev );
    
    CREATE( cargo, CARGO_DATA, 1 );
    
    do // Checks for duplicates of cargo type in spaceobject 
          // ( Come up with better way - DV 3-16-04 )
    {
      dup = FALSE;

      cargo->cargotype = number_range( 0, CARGOTYPE_DEFAULT-1 );
      
      if ( spaceobject->first_cargo )
        for ( cargoscroll = spaceobject->first_cargo; cargoscroll && cargoscroll->cargo && dup == FALSE; cargoscroll = cargoscroll->next )
        {
          dup = ( cargo->cargotype == cargoscroll->cargo->cargotype );
        }
      
    } while ( dup == TRUE );

    defprice = cargodefaults[cargo->cargotype].price;
    
    calcrange = ((int) sqrt(defprice))/2;

    cargo->price = defprice + number_range( -1*defprice/calcrange, defprice/calcrange );

    cargolist->cargo = cargo;
    
    return TRUE;

}

#define SPACEOBJECT_CARGOAMOUNT 3
bool randomize_spaceobject_cargo( SPACE_DATA *spaceobject )
{
    bool adding = TRUE;
    CARGO_DATA_LIST *cargolist;
    int i;

    if ( spaceobject->first_cargo )
    {
      for( cargolist = spaceobject->first_cargo; cargolist; cargolist = cargolist->next )
        adding = set_random_cargo( spaceobject, cargolist ); // Randomize cargo
    }
    else
    {
      for( i = 0; (i<SPACEOBJECT_CARGOAMOUNT) && (adding == TRUE); i++ ) // Add random cargo to spaceobject
        adding = add_random_cargo( spaceobject );
    }
    
    return adding;
}



void do_cargo( CHAR_DATA *ch, char *argument )
{
    
    int cargocount = 0, cargolistnum, defprice, cargotype;
    std::string arg1;
    std::string arg2;
    std::string arg3;
    std::string arg4;
    std::string arg5;
    std::string argstr = argument;
    std::string hilow;
    
    SPACE_DATA *spaceobject = NULL;
    CARGO_DATA_LIST *cargolist;

    if ( argstr.empty() )
    {
        send_to_char( "\nSyntax: loadcargo <argument> <argument>...\n", ch );
        send_to_char( "Arguments: planet <spaceobjectname> <argument>..., search\n", ch );
        send_to_char( "Planet arguments: randomize, set <#list> <argument> <number>, stats\n", ch );
        send_to_char( "Planet set arguments: type <type#>, price <amount>\n", ch );
        send_to_char( "search arguments: overprice, underprice, cargotype <type#>\n", ch );
        return;
    }

    argstr = one_argument( argstr, arg1 );
    argstr = one_argument( argstr, arg2 );

    if ( arg1.empty() || arg2.empty() )
    {
        send_to_char( "\nSyntax: cargo <argument> <argument>...\n", ch );
        send_to_char( "Arguments: planet <spaceobjectname> <argument>..., search\n", ch );
        send_to_char( "Planet arguments: randomize, set <#list> <argument> <number>, stats\n", ch );
        send_to_char( "Planet set arguments: type <type#>, price <amount>\n", ch );
        send_to_char( "search arguments: overprice, underprice, cargotype <type#>\n", ch );
        return;
    }
    
    if ( !str_cmp( arg1, "planet" ) )
    {
        if ( argstr.empty() )
        {
            send_to_char( "No argument: cargo planet <spaceobjname> <argument>...\n", ch );
            return;
        }
          
        argstr = one_argument( argstr, arg3 );
        spaceobject = spaceobject_from_name( ch->game, arg2 );
        if ( !spaceobject )
        {
            ch_printf( ch, "Starsystem %s not found.\n", arg2.c_str() );
            return;
        }
        
        if ( !str_cmp( arg3, "randomize" ) )
        {
            if ( !randomize_spaceobject_cargo( spaceobject ) )
                send_to_char( "Error encountered in randomizing cargo.\n", ch );
            ch_printf( ch, "Cargo data on planet %s randomized.\n", spaceobject->name );
            save_spaceobject( spaceobject );

            return;
        }
        if ( !str_cmp( arg3, "stats" ) )
        {
            if ( !spaceobject->first_cargo )
            {
                send_to_char( "This spaceobject has no cargo set.\n", ch );
                return;
              }
            ch_printf( ch, "Cargo Data: Spaceobject %s.\n", spaceobject->name );
            for( cargolist = spaceobject->first_cargo; cargolist; cargolist = cargolist->next )
            {
                cargocount++;
                if ( !cargolist->cargo )
                {
                  ch_printf( ch, "Error: cargolist->cargo does not exist on spaceobject %s.\n", spaceobject->name );
                  return;
                }
          
                defprice = cargodefaults[cargolist->cargo->cargotype].price;

                if ( cargolist->cargo->price > defprice + (int) sqrt(defprice)/4)
                    hilow = "High";
                else if ( cargolist->cargo->price < defprice - (int) sqrt(defprice)/4)
                    hilow = "Low";
                else
                    hilow = "Med";
                
                ch_printf( ch, "%d) Cargo: %.12s Price: %d (%s).\n", 
                cargocount, get_cargo_name( cargolist->cargo->cargotype ).c_str(), 
                cargolist->cargo->price,
                hilow.c_str() );
            }
            return;
        }
        
        if ( !str_cmp( arg3, "set" ) )
        {
            if ( !spaceobject->first_cargo )
            {
                send_to_char( "This spaceobject has no cargo set.\n", ch );
                return;
            }
            if ( argstr.empty() )
            {
                send_to_char( "Missing #list and beyond.&R&w\n", ch );
                return;
            }
          
            argstr = one_argument( argstr, arg4 );

            if ( !is_number( arg4 ) )
            {
                send_to_char( "#list needs to be a number.\n", ch );
                return;
            }
            if ( argstr.empty() )
            {
                send_to_char( "Missing set field: <argument> and beyond.\n", ch );
                return;
            }
            argstr = one_argument( argstr, arg5 );
            if ( argstr.empty() )
            {
                send_to_char( "Missing set field: <type>/<amount>.\n", ch );
                return;
            }
            if ( !is_number( argstr ) )
            {
                send_to_char( "<type>/<amount> needs to be a number.\n", ch );
                return;
            }
            cargolistnum = strtoi(arg4);
            cargolist = spaceobject->first_cargo;
            for( cargocount = 0; cargocount < cargolistnum; cargocount++ )
            {
                if ( !cargolist )
                  break;

                if ( !cargolist->cargo )
                {
                  ch_printf( ch, "Error: cargolist->cargo does not exist on spaceobject %s.\n", spaceobject->name );
                  return;
                }
                cargolist = cargolist->next;
            }
            if ( !cargolist )
            {
                ch_printf( ch, "Not that many cargo items on spaceobject %s.\n", spaceobject->name );
                return;
            }
            if ( !str_cmp( arg5, "type" ) )
            {
                cargolist->cargo->cargotype = strtoi(argstr);
                send_to_char( "Done.\n", ch );
                save_spaceobject( spaceobject );
                return;
            }      
            if ( !str_cmp( arg5, "price" ) )
            {
                cargolist->cargo->price = strtoi(argstr);
                send_to_char( "Done.\n", ch );
                save_spaceobject( spaceobject );
                return;
            }
            send_to_char( "No such option.  Options: type, price.\n", ch );
            return;
        }
        send_to_char( "No such option.  Options: randomize, set, stats.\n", ch );
        return;

    }
    
    if ( !str_cmp( arg1, "search" ) )
    {
      
        if ( !str_cmp( arg2, "cargotype" ) )
        {
            if ( argstr.empty() || !is_number(argstr) )
            {
              send_to_char( "No cargotype value: cargo search cargotype <cargotype#>\n", ch );
              return;
            }
            
            cargotype = strtoi(argstr);
            ch_printf( ch, "Results for cargo type %s\n", get_cargo_name( cargotype ).c_str() );
            for ( spaceobject = first_spaceobject; spaceobject; spaceobject = spaceobject->next )
              if ( spaceobject->first_cargo )
                for ( cargolist = spaceobject->first_cargo; cargolist; cargolist = cargolist->next )
                  if ( cargolist->cargo->cargotype == cargotype )
                    ch_printf( ch, "%s: Price: %d\n", spaceobject->name, cargolist->cargo->price );
            return;            

        }    
        send_to_char( "Not yet implemented.\n", ch );
        return;
      
    }

    send_to_char( "\nSyntax: cargo <argument> <argument>...\n", ch );
    send_to_char( "Arguments: planet <spaceobjectname> <argument>..., search\n", ch );
    send_to_char( "Planet arguments: randomize, set <#list> <argument> <number>, stats\n", ch );
    send_to_char( "Planet set arguments: type <type#>, price <amount>\n", ch );
    send_to_char( "search arguments: overprice, underprice, cargotype <type#>\n", ch );
    return;
    
    
    return;
}

void do_repair_module ( CHAR_DATA *ch, char *argument ) // Coded by Johnson ( Michael Shattuck ) - Added 5-15-04 - DV
{
    std::string arg;
    OBJ_DATA *obj;
    int chance;

    one_argument( argument, arg );

    switch( ch->substate )
    {
    	default:
    	       
        chance = IS_NPC(ch) ? ch->top_level
            : (int) (ch->pcdata->learned[gsn_repairmodule]);

				if( chance <= 0 )
				{
            send_to_char( "You do not know how to repair a module.\n", ch );
            return;
				}

				if ( ms_find_obj(ch) )
					  return;

				if ( ( obj = get_obj_carry( ch, arg ) ) == NULL )
				{
            send_to_char( "You do not have that item.\n", ch );
            return;
				}

				if ( !( obj->item_type==ITEM_FIGHTERCOMP || obj->item_type==ITEM_MIDCOMP
					|| obj->item_type==ITEM_CAPITALCOMP ) )
				{
            send_to_char("That isn't a ship module.\n",ch);
            return;
				}

				if ( obj->value[0] >= 100 )
					chance = (int) ( chance*0.50 );

        if ( number_percent( ) < chance )
        {
            send_to_char( "&GYou begin your repairs\n", ch);
            act( AT_PLAIN, "$n begins repairing a ship module.", ch, NULL, arg , TO_ROOM );
            add_timer ( ch , TIMER_DO_FUN , 5 , do_repair_module , 1 );
            ch->dest_buf = str_dup(arg);
            return;
				}

				send_to_char("&RYou fail to locate the source of the problem.\n",ch);

				if ( obj->value[0] >= 100 )
				{
            send_to_char("&RYou failed overcharging the module and cause serious damage!.\n",ch);
            obj->value[0] = (int) ((ch->pcdata->learned[gsn_repairmodule]) * 0.75 );
				}
				else
				{
					if ( number_percent( ) > URANGE( 50, chance * 1.25, 100  ) )
					{
              send_to_char("&RYou slip up and damage it slightly!\n",ch);
              obj->value[0] += (int) (1 - URANGE(1, number_percent( )/20, 5 ));
					}
				}
				
				learn_from_failure( ch, gsn_repairmodule );
    	   		return;

    	case 1:
          if ( !ch->dest_buf )
            return;
          arg = (const char * )ch->dest_buf;
          STR_DISPOSE( ch->dest_buf);
          break;

    	case SUB_TIMER_DO_ABORT:
          STR_DISPOSE( ch->dest_buf );
          ch->substate = SUB_NONE;
            send_to_char("&RYou are distracted and fail to finish your repairs.\n", ch);
          return;
    }

    if ( ms_find_obj(ch) )
        return;

    if ( ( obj = get_obj_carry( ch, arg ) ) == NULL )
    {
        send_to_char( "The item you were working on seems to be gone...\n", ch );
        return;
    }

	ch->substate = SUB_NONE;
	act( AT_PLAIN, "$n finishes the repairs.", ch, NULL, argument , TO_ROOM );


	obj->value[0] += (int) ((ch->pcdata->learned[gsn_repairmodule]) * 0.05 );
	if ( obj->value[0] < 100 )
	    obj->value[0] += (int) ((ch->pcdata->learned[gsn_repairmodule]) * 0.15 );

	if ( obj->value[0] >= 125 )
	{
      send_to_char("&RYou cannot repair it more than 125%!\n", ch);
      obj->value[0] = 125;
	}
	else
	{
	    send_to_char("&GRepair successful.\n", ch);
	}

	learn_from_success( ch, gsn_repairmodule );

	return;
	
}

void do_checkareaships ( CHAR_DATA *ch, char *argument )
{
    // Checks for ships in an area.
    SHIP_DATA *ship;
    AREA_DATA *tarea;
    int shipsfound = 0, roomvnum;
    
    for ( tarea = first_area; tarea; tarea = tarea->next )
    {
        if ( !str_cmp( tarea->filename, argument ) )
          break;
    }
      
    if ( !tarea )
    {
        send_to_char( "Area not found.\n", ch );
        return;
    }
    
    for ( roomvnum = tarea->low_r_vnum; roomvnum < tarea->hi_r_vnum; roomvnum++ )
    {    
        if( ( ship = ship_from_cockpit( ch->game, roomvnum ) ) != NULL )
        {
            ch_printf( ch, "Ship: %s %s, Room: %d\n", ship->name, ship->personalname, roomvnum );
            shipsfound++;
        }
    }
    
    ch_printf( ch, "%d rooms found.\n", shipsfound );
    return;

}
