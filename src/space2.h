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
*                           Space Module 2 Include                         *
****************************************************************************/

#include <math.h>
#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#define MAX_TEMPLATETYPE 38

SHIP_DATA *get_ship_in_space( GameContext *game, const std::string& name );

struct scmd_data
{
    SHIP_DATA *ship = nullptr;
    SHIP_DATA *target = nullptr;
    ROOM_INDEX_DATA *room = nullptr;
    std::string arg1;
    std::string arg2;
    std::string arg3;
    std::string arg4;
    std::string arg5;
    std::string rest;
};



inline bool space_require_ship_from_cockpit( CHAR_DATA *ch, scmd_data &ctx, bool showmsg = true )
{
    ctx.ship = ship_from_cockpit( ch->game, ch->in_room->vnum );
    if ( !ctx.ship )
    {
        if (showmsg)
            send_to_char( "&RYou must be in the cockpit of a ship to do that!\n", ch );
        return false;
    }
    return true;
}

inline bool space_require_ship_from_pilotseat( CHAR_DATA *ch, scmd_data &ctx, bool showmsg = true )
{
    ctx.ship = ship_from_pilotseat( ch->game, ch->in_room->vnum );
    if ( !ctx.ship )
    {
        if (showmsg)
            send_to_char( "&RYou don't seem to be in the pilot seat!\n", ch );
        return false;
    }
    return true;
}

inline bool space_require_ship_from_navseat( CHAR_DATA *ch, scmd_data &ctx, bool showmsg = true )
{
    ctx.ship = ship_from_navseat( ch->game, ch->in_room->vnum );
    if ( !ctx.ship )
    {
        if (showmsg)
            send_to_char( "&RYou don't seem to be in the navigator seat!\n", ch );
        return false;
    }
    return true;
}

inline bool space_require_ship_from_engineroom( CHAR_DATA *ch, scmd_data &ctx, bool showmsg = true )
{
    ctx.ship = ship_from_engine( ch->game, ch->in_room->vnum );
    if ( !ctx.ship )
    {
        if (showmsg)
            send_to_char( "&RYou don't seem to be in the engine room!\n", ch );
        return false;
    }
    return true;
}

inline bool space_require_ship_from_hanger( CHAR_DATA *ch, scmd_data &ctx, bool showmsg = true )
{
    ctx.ship = ship_from_hanger( ch->game, ch->in_room->vnum );
    if ( !ctx.ship )
    {
        if (showmsg)
            send_to_char( "&RYou don't seem to be in the hanger!\n", ch );
        return false;
    }
    return true;
}

inline bool space_require_ship_from_turret( CHAR_DATA *ch, scmd_data &ctx, bool showmsg = true )
{
    ctx.ship = ship_from_turret( ch->game, ch->in_room->vnum );
    if ( !ctx.ship )
    {
        if (showmsg)
            send_to_char( "&RYou must be in the gunners seat or turret of a ship to do that!\n", ch );
        return false;
    }
    return true;
}

inline bool space_requires_finish_launching( CHAR_DATA *ch, scmd_data &ctx, bool showmsg = true )
{
    if (! ctx.ship->spaceobject && ctx.ship->shipclass <= SHIP_PLATFORM)
    {
        if (showmsg)
            send_to_char("&RYou can't do that until you've finished launching!\n",ch);
        return false;
    }
    return true;
}

inline bool space_requires_in_system( CHAR_DATA *ch, scmd_data &ctx, bool showmsg = true )
{
    if (ctx.ship->spaceobject == NULL)
    {
        if (showmsg)
            send_to_char("&RYou can't do that unless the ship is flying in realspace!\n",ch);
        return false;
    }        
    return true;
}

inline bool space_require_ship_from_coseat( CHAR_DATA *ch, scmd_data &ctx, bool showmsg = true )
{
    ctx.ship = ship_from_coseat( ch->game, ch->in_room->vnum );
    if ( !ctx.ship )
    {
        if (showmsg)
            send_to_char( "&RYou don't seem to be in the co-pilot seat!\n", ch );
        return false;
    }
    return true;
}

inline bool space_require_ship_from_pilot_or_hanger( CHAR_DATA *ch, scmd_data &ctx, bool showmsg = true )
{
    if ( space_require_ship_from_pilotseat( ch, ctx, false ) )
        return true;
    if ( space_require_ship_from_hanger( ch, ctx, false ) )
        return true;

    if (showmsg)
        send_to_char("&RYou aren't in the pilots chair or hanger of a ship!\n",ch);
    return false;
}

inline bool space_require_ship_has_hanger( CHAR_DATA *ch, scmd_data &ctx, bool showmsg = true )
{
    if ( ctx.ship->hanger == 0 )
    {
        if (showmsg)
            send_to_char("&RThis ship has no hanger!\n",ch);
        return false;
    }
    return true;
}

inline bool space_require_ship_bay_closed( CHAR_DATA *ch, scmd_data &ctx, bool showmsg = true )
{
    if ( ctx.ship->bayopen == TRUE )
    {
        if (showmsg)
            send_to_char("Bay doors are already open!",ch);
        return false;
    }
    return true;
}

inline bool space_require_ship_bay_open( CHAR_DATA *ch, scmd_data &ctx, bool showmsg = true )
{
    if ( ctx.ship->bayopen == FALSE )
    {
        if (showmsg)
            send_to_char("Bay doors are already closed!", ch);
        return false;
    }
    return true;
}

inline bool space_require_ship_launch_ready_state( CHAR_DATA *ch, scmd_data &ctx, bool showmsg = true )
{
    if ( ctx.ship->shipstate != SHIP_LANDED && ctx.ship->shipstate != SHIP_DISABLED )
    {
        if (showmsg)
            send_to_char("The ship is not fully docked right now.\n",ch);
        return false;
    }
    return true;
}

inline bool space_require_ship_notplatform( CHAR_DATA *ch, scmd_data &ctx, bool showmsg = true )
{
    if  ( ctx.ship->shipclass == SHIP_PLATFORM )
    {
        if (showmsg)
            send_to_char( "&RPlatforms can not move!\n" , ch );
        return false;
    }
    return true;
}

inline bool space_require_ship_nottractored( CHAR_DATA *ch, scmd_data &ctx, bool showmsg = true )
{
    if ( ctx.ship->tractoredby )
    {
        if (showmsg)
            send_to_char("&RYou are still locked in a tractor beam!\n",ch);
        return false;
    }
    return true;
}

inline bool space_require_ship_not_docked( CHAR_DATA *ch, scmd_data &ctx, bool showmsg = true )
{
    if ( ctx.ship->docking != SHIP_READY )
    {
        if (showmsg)
            send_to_char("&RYou can't do that while docked to another ship!\n",ch);
        return false;
    }
    return true;
}

inline bool space_require_ship_energy( CHAR_DATA *ch, scmd_data &ctx, int energy_required = 0, bool showmsg = true )
{
    if (ctx.ship->energy <= energy_required)
    {
      if (showmsg)
          send_to_char("&RThere is not enough fuel!\n",ch);
      return false;
    }	      
    return true;
}

inline bool space_require_tractor_weaker( CHAR_DATA *ch, scmd_data &ctx, bool showmsg = true )
{
    if (ctx.ship->tractored && ctx.ship->tractored->shipclass > ctx.ship->shipclass )
    {
        if (showmsg)
            send_to_char("&RYou can not move while a tractorbeam is locked on to such a large mass.\n",ch);
        return false;
    }
    return true;
}

inline bool space_require_ship_not_landed( CHAR_DATA *ch, scmd_data &ctx, bool showmsg = true )
{
    if ( ctx.ship->shipstate == SHIP_LANDED )
    {
        if (showmsg)
            send_to_char("&RYou can't do that until after you've launched!\n",ch);
        return false;
    }
    return true;
}

inline bool space_require_ship_tractoree_ed( CHAR_DATA *ch, scmd_data &ctx, bool showmsg = true )
{
    if ( ctx.ship->tractoredby )
    {
        if (showmsg)
            send_to_char("&RYou are still locked in a tractor beam!\n",ch);
        return false;
    }  
    if (ctx.ship->tractored && ctx.ship->tractored->shipclass > ctx.ship->shipclass )
    {
        if (showmsg)
            send_to_char("&RYou can not move while a tractorbeam is locked on to such a large mass.\n",ch);
        return false;
    }
    return true;
}

inline bool space_require_ship_movement( CHAR_DATA *ch, scmd_data &ctx, bool showmsg = true )
{
    if ( !space_require_ship_notplatform( ch , ctx , showmsg ) )
        return false;
    if ( !space_require_ship_not_docked( ch , ctx , showmsg ) )
        return false;
    if ( !space_require_ship_energy( ch , ctx , 0 , showmsg ) )
        return false;

    return true;
}

inline bool space_require_ship_spaceship( CHAR_DATA *ch, scmd_data &ctx, bool showmsg = true )
{
    if ( ctx.ship->shipclass > SHIP_PLATFORM )
    {
        if (showmsg)
            send_to_char("&RThis isn't a spacecraft!\n",ch);
        return false;
    }
    return true;
}

inline bool space_require_ship_ownership( CHAR_DATA *ch, scmd_data &ctx, bool showmsg = true )
{
    if ( !check_pilot( ch , ctx.ship ) )
    {
        if (showmsg)
            send_to_char("&RYou don't have permission to do that!\n",ch);
        return false;
    }
    return true;
}

inline bool space_require_ship_autopilot_off( CHAR_DATA *ch, scmd_data &ctx, bool showmsg = true )
{
    if ( autofly(ctx.ship) )
    {
        if (showmsg)
          send_to_char("&RThe ship is set on autopilot, you'll have to turn it off first.\n",ch);
        return false;
    }
    return true;
}

inline bool space_require_ship_docked( CHAR_DATA *ch, scmd_data &ctx, bool showmsg = true )
{
    if ( ctx.ship->lastdoc != ctx.ship->location )
    {
        if (showmsg)
            send_to_char("&RYou don't seem to be docked right now.\n",ch);
        return false;
    }
    return true;
}

inline bool space_require_ship_not_capital( CHAR_DATA *ch, scmd_data &ctx, bool showmsg = true )
{
    if ( ctx.ship->shipclass == CAPITAL_SHIP )
    {
        if (showmsg)
            send_to_char("&RCapital ships can't do that!\n",ch);
        return false;
    }
    return true;
}

inline bool space_require_ship_not_disabled( CHAR_DATA *ch, scmd_data &ctx, bool showmsg = true )
{
    if ( ctx.ship->shipstate == SHIP_DISABLED )
    {
        if (showmsg)
            send_to_char("&RYour ship is disabled and can't do that!\n",ch);
        return false;
    }
    return true;
}
// The below function verifies that the ship is not landed or docked with another ship
inline bool space_require_ship_not_landed_or_docked( CHAR_DATA *ch, scmd_data &ctx, bool showmsg = true )
{
    if ( ctx.ship->shipstate == SHIP_LANDED && ctx.ship->docked == NULL )
    {
        if (showmsg)
            send_to_char("&RYou can't do that while docked!\n",ch);
        return false;
    }
    return true;
}

inline bool space_require_ship_not_hyperspace( CHAR_DATA *ch, scmd_data &ctx, bool showmsg = true )
{
    if ( ctx.ship->shipstate == SHIP_HYPERSPACE )
    {
        if (showmsg)
            send_to_char("&RYou can't do that while in hyperspace!\n",ch);
        return false;
    }
    return true;
}

inline bool space_require_ship_ready( CHAR_DATA *ch, scmd_data &ctx, bool showmsg = true )
{
    if ( ctx.ship->shipstate != SHIP_READY )
    {
        if (showmsg)
            send_to_char("&RPlease wait until the ship has finished its current manouver.\n",ch);
        return false;
    }
    return true;
}

inline bool space_require_target_hangar_valid( CHAR_DATA *ch, scmd_data &ctx, bool showmsg = true )
{
    if ( ctx.target == NULL )
    {
        if (showmsg)
            send_to_char("&RI don't see that here. Type land by itself for a list\n",ch);
        return false;
    }
    if ( ctx.target == ctx.ship )
    {
        if (showmsg)
          send_to_char("&RYou can't land your ship inside itself!\n",ch);
        return false;
    }
    if ( ! ctx.target->hanger )
    {
        if (showmsg)
            send_to_char("&RThat ship has no hanger for you to land in!\n",ch);
        return false;
    }
    if ( ctx.ship->shipclass == MIDSIZE_SHIP && ctx.target->shipclass == MIDSIZE_SHIP )
    {
        if (showmsg)
            send_to_char("&RThat ship is not big enough for your ship to land in!\n",ch);
        return false;
    }
    if ( ! ctx.target->bayopen )
    {
        if (showmsg)
            send_to_char("&RTheir hanger is closed. You'll have to ask them to open it for you\n",ch);
        return false;
    }
    return true;
}

enum class ProjectileKind
{
    MISSILE,
    TORPEDO,
    ROCKET
};

inline bool space_validate_tractor_links( CHAR_DATA *ch, SHIP_DATA *ship, bool showmsg = true )
{
    bool ok = true;

    if ( ship->tractored && ship->tractored->tractoredby != ship )
    {
        ship->tractored = NULL;
        ok = false;
    }

    if ( ship->tractoredby && ship->tractoredby->tractored != ship )
    {
        ship->tractoredby = NULL;
        if ( ship->shipstate == SHIP_TRACTORED )
            ship->shipstate = SHIP_READY;
        ok = false;
    }

    if ( !ok && showmsg )
        send_to_char("&RTractor lock desync detected. Control link reset.\n", ch);

    return ok;
}

static inline SHIP_DATA *space_get_turret_target( CHAR_DATA *ch, TURRET_DATA *turret, bool showmsg = true )
{
    if ( turret->target == NULL )
    {
        if (showmsg)
            send_to_char( "&RYou need to choose a target first.\n", ch );
        return NULL;
    }

    return turret->target;
}

static inline SHIP_DATA *space_get_target( CHAR_DATA *ch, SHIP_DATA *ship, bool showmsg = true )
{
    if ( ship->target0 == NULL )
    {
        if (showmsg)
            send_to_char( "&RYou need to choose a target first.\n", ch );
        return NULL;
    }

    return ship->target0;
}

static inline bool target_left_or_clear( CHAR_DATA *ch, SHIP_DATA *ship, SHIP_DATA *target, SHIP_DATA **target_slot )
{
    if ( ship->shipclass <= SHIP_PLATFORM && !ship_in_range( ship, target ) )
    {
        send_to_char( "&RYour target seems to have left.\n", ch );
        *target_slot = NULL;
        return true;
    }

    if ( ship->shipclass > SHIP_PLATFORM && target->in_room != ship->in_room )
    {
        send_to_char( "&RYour target seems to have left.\n", ch );
        *target_slot = NULL;
        return true;
    }

    return false;
}

static inline int ship_target_distance( SHIP_DATA *from, SHIP_DATA *to )
{
    return ( abs( (int)( to->vx - from->vx ) )
           + abs( (int)( to->vy - from->vy ) )
           + abs( (int)( to->vz - from->vz ) ) ) / 3;
}

static inline bool target_out_of_coord_range( CHAR_DATA *ch, SHIP_DATA *ship, SHIP_DATA *target, int max_delta, const char *msg )
{
    if ( ship->shipclass > SHIP_PLATFORM )
        return false;

    if ( abs( (int)( target->vx - ship->vx ) ) > max_delta
      || abs( (int)( target->vy - ship->vy ) ) > max_delta
      || abs( (int)( target->vz - ship->vz ) ) > max_delta )
    {
        if (msg)
            send_to_char( msg, ch );
        return true;
    }

    return false;
}

inline bool space_distance_ship_less_than( SHIP_DATA *ship1, SHIP_DATA *ship2, long distance )
{
    if ( abs( (int) ( ship1->vx - ship2->vx )) > distance ||
    abs( (int) ( ship1->vy - ship2->vy )) > distance ||
    abs( (int) ( ship1->vz - ship2->vz )) > distance )
        return false;
    return true;
}

static inline void maybe_autofly_retarget( SHIP_DATA *attacker, SHIP_DATA *target )
{
    if ( target && autofly( target ) && target->target0 != attacker && attacker->spaceobject )
    {
        target->target0 = attacker;
        echo_to_cockpit( AT_BLOOD, attacker,
                         str_printf( "You are being targetted by %s.", target->name ) );
    }
}

static inline void echo_ship_observers( int at_color, SHIP_DATA *ship, SHIP_DATA *target, const std::string &msg )
{
    if ( ship->shipclass > SHIP_PLATFORM )
        echo_to_room( at_color, ship->in_room, msg );
    else
        echo_to_system( at_color, ship, msg, target );
}

static inline int compute_projectile_hit_chance( int chance, int origchance, SHIP_DATA *ship, SHIP_DATA *target )
{
    int distance = ship_target_distance( ship, target );

    chance += target->shipclass - ship->shipclass;
    chance += ship->currspeed - target->currspeed;
    chance += ship->mod->manuever - target->mod->manuever;
    chance -= distance / ( 10 * ( target->shipclass + 1 ) );
    chance -= origchance;
    chance /= 2;
    chance += origchance;

    chance += 30;
    return URANGE( 20, chance, 99 );
}

inline int space_chance_by_shipclass( CHAR_DATA *ch, SHIP_DATA *ship )
{
    int chance = 50;
     if ( ship->shipclass <= FIGHTER_SHIP )
          chance = IS_NPC(ch) ? ch->top_level
          : (int)  (ch->pcdata->learned[gsn_starfighters]) ;
    if ( ship->shipclass == MIDSIZE_SHIP )
        chance = IS_NPC(ch) ? ch->top_level
        : (int)  (ch->pcdata->learned[gsn_midships]) ;
    if ( ship->shipclass == CAPITAL_SHIP )
        chance = IS_NPC(ch) ? 0 // CHanged so mobs can't fly capital ships - otherwise possession would allow players to bypass requirements.
        : (int) (ch->pcdata->learned[gsn_capitalships]);
    return chance;
}

inline int space_chance_combat( CHAR_DATA *ch, SHIP_DATA *ship = nullptr)
{
    int chance;
    chance = IS_NPC(ch) ? ch->top_level
            : (int) ( ch->perm_dex*2 + ch->pcdata->learned[gsn_spacecombat]/3
                    + ch->pcdata->learned[gsn_spacecombat2]/3 + ch->pcdata->learned[gsn_spacecombat3]/3 );        
    return chance;
}

inline int space_chance_by_shipsystems( CHAR_DATA *ch, SHIP_DATA *ship )
{
    int chance;
    chance = IS_NPC(ch) ? ch->top_level
        : (int)  (ch->pcdata->learned[gsn_shipsystems]) ;
    return chance;
}


inline void space_learn_from_success( CHAR_DATA *ch, SHIP_DATA *ship )
{
    if ( ship->shipclass <= FIGHTER_SHIP )
        learn_from_success( ch , gsn_starfighters );
    else if ( ship->shipclass == MIDSIZE_SHIP )
        learn_from_success( ch , gsn_midships );
    else if ( ship->shipclass == CAPITAL_SHIP )
        learn_from_success( ch , gsn_capitalships );
}

inline void space_learn_from_failure( CHAR_DATA *ch, SHIP_DATA *ship )
{
    if ( ship->shipclass <= FIGHTER_SHIP )
        learn_from_failure( ch , gsn_starfighters );
    else if ( ship->shipclass == MIDSIZE_SHIP )
        learn_from_failure( ch , gsn_midships );
    else if ( ship->shipclass == CAPITAL_SHIP )
        learn_from_failure( ch , gsn_capitalships );
}

inline void space_learn_combat_from_success( CHAR_DATA *ch )
{
    learn_from_success( ch , gsn_spacecombat );
    learn_from_success( ch , gsn_spacecombat2 );
    learn_from_success( ch , gsn_spacecombat3 );
}

inline void space_learn_combat_from_failure( CHAR_DATA *ch )
{
    learn_from_failure( ch , gsn_spacecombat );
    learn_from_failure( ch , gsn_spacecombat2 );
    learn_from_failure( ch , gsn_spacecombat3 );
}

inline void space_show_landing_choices( CHAR_DATA *ch, SHIP_DATA *ship )
{
    SPACE_DATA *spaceobj;
    ch_printf(ch, "%s", "Choices:\n" );
    for( spaceobj = first_spaceobject; spaceobj; spaceobj = spaceobj->next )
    {
        if( space_in_range( ship, spaceobj ) )
        {
            if ( spaceobj->doca && !spaceobj->seca)
                ch_printf(ch, "%s (%s)  %.0f %.0f %.0f\n         " ,
                    spaceobj->locationa,
                    spaceobj->name,
                    spaceobj->xpos,
                    spaceobj->ypos,
                    spaceobj->zpos );
            if ( spaceobj->docb && !spaceobj->secb )
                ch_printf(ch, "%s (%s)  %.0f %.0f %.0f\n         " ,
                    spaceobj->locationb,
                    spaceobj->name,
                    spaceobj->xpos,
                    spaceobj->ypos,
                    spaceobj->zpos );
            if ( spaceobj->docc && !spaceobj->secc )
                ch_printf(ch, "%s (%s)  %.0f %.0f %.0f\n         " ,
                    spaceobj->locationc,
                    spaceobj->name,
                    spaceobj->xpos,
                    spaceobj->ypos,
                    spaceobj->zpos );
        }
    }
}

enum class ShipBuyContext
{
    PLAYER,
    CLAN
};

static inline CLAN_DATA *get_mainclan( CLAN_DATA *clan )
{
    return ( clan && clan->mainclan ) ? clan->mainclan : clan;
}

static inline bool ship_is_owned( SHIP_DATA *ship )
{
    return str_cmp( ship->owner, "" ) || ship->type == MOB_SHIP;
}

static inline SHIP_DATA *find_ship_in_or_room( CHAR_DATA *ch, const char *argument, bool allow_cockpit )
{
    SHIP_DATA *ship = ship_in_room( ch->in_room, argument );

    if ( !ship && allow_cockpit )
        ship = ship_from_cockpit( ch->game, ch->in_room->vnum );

    if ( !ship )
    {
        if ( !argument || argument[0] == '\0' )
             send_to_char( "Which ship do you want?\n", ch );
        else
            act( AT_PLAIN, "I see no $T here.", ch, NULL, argument, TO_CHAR );
    }
    return ship;
}

static inline void ship_show( CHAR_DATA *ch, SHIP_DATA *ship, bool fromafar = false )
{
    ch_printf( ch, "&Y%s %s : %s (%s)\n&B",
        ship->type == SHIP_REBEL ? "Rebel" :
        (ship->type == SHIP_IMPERIAL ? "Imperial" : "Civilian" ),
        ship->shipclass == FIGHTER_SHIP ? "Starfighter" :
        (ship->shipclass == MIDSIZE_SHIP ? "Midtarget" : 
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
        ship->filename);
    ch_printf( ch, "Description: %s\nOwner: %s",
        ship->description,
        ship->owner );
    if( fromafar == FALSE )
        ch_printf( ch, "   Pilot: %s   Copilot: %s", ship->pilot,  ship->copilot );

    ch_printf( ch, "\nLaser cannons: %d  Ion cannons: %d\n",
        ship->mod->lasers, ship->mod->ions);
    ch_printf( ch, "Projectile Launchers (8): %d  ",
        ship->mod->launchers);
    ch_printf( ch, "Chaff Launchers (6): %d\n",
        ship->mod->defenselaunchers);
    ch_printf( ch, "Max Hull: %d  ",
        ship->mod->maxhull);
    ch_printf( ch, "Max Shields: %d   Max Energy(fuel): %d\n",
        ship->mod->maxshield,
        ship->mod->maxenergy);
    ch_printf( ch, "Maximum Speed: %d   Hyperspeed: %d  Value: %d\n",
        ship->mod->realspeed, ship->mod->hyperspeed, get_ship_value( ship ));
    ch_printf( ch, "Maximum Cargo: %d\n", ship->maxcargo );

}

static inline void ship_show_status( CHAR_DATA *ch, SHIP_DATA *ship )
{
    TURRET_DATA *turret;
    int turretnum = 0;
    ch_printf( ch, "&W%s:\n",ship->name);
    ch_printf( ch, "&OCurrent Coordinates:&Y %.0f %.0f %.0f\n",
                        ship->vx, ship->vy, ship->vz );
    ch_printf( ch, "&OCurrent Heading:&Y %.0f %.0f %.0f\n",
                        ship->hx, ship->hy, ship->hz );
    ch_printf( ch, "&OCurrent Speed:&Y %d&O/%d Accel: &Y%d&O/%d\n",
                        ship->currspeed , ship->mod->realspeed, 
                        ship->accel, get_acceleration(ship) );
    ch_printf( ch, "&OHull:&Y %d&O/%d  Ship Condition:&Y %s\n",
                        ship->hull,
    		                ship->mod->maxhull,
    			              ship->shipstate == SHIP_DISABLED ? "Disabled" : "Running");
    ch_printf( ch, "&OShields:&Y %d&O/%d   Energy(fuel):&Y %d&O/%d\n",
                        ship->shield,
                        ship->mod->maxshield,
                        ship->energy,
                        ship->mod->maxenergy);
    ch_printf( ch, "&OLaser Condition:&Y %s  &OCurrent Target:&Y %s\n",
    		                ship->statet0 == LASER_DAMAGED ? "Damaged" : "Good" , ship->target0 ? ship->target0->name : "none");
    if (ship->first_turret)
        for ( turret = ship->first_turret, turretnum = 0; turret; turret= turret->next, turretnum++ )
            ch_printf( ch, "&OTurret %d:  &Y %s  &OCurrent Target:&Y %s\n",
    			      turretnum, turret->state == LASER_DAMAGED ? "Damaged" : "Good" , turret->target ? turret->target->name : "none");

    ch_printf( ch, "&OSensors:    &Y%d   &OTractor Beam:   &Y%d\n", ship->mod->sensor, ship->mod->tractorbeam);
    ch_printf( ch, "&OAstroArray: &Y%d   &OComm:           &Y%d\n", ship->mod->astro_array, ship->mod->comm);
    if( ship->mod->gravitypower > 0 )
        ch_printf( ch, "&OGravity Well Projectors: Power: &Y%d\n", ship->mod->gravitypower );
    ch_printf( ch, "\n&OMissiles:&Y %d&O  Torpedos: &Y%d&ORockets:  &Y%d&O\n  Chaff:    &Y%d&O  Gravity Power: %d/%d\n Launchers: %d/%d Condition:&Y %s&w\n",
       			ship->missiles,
    			  ship->torpedos,
    			  ship->rockets,
            ship->chaff, ship->mod->gravproj, ship->mod->gravitypower,
            ship->mod->launchers,
            ship->mod->defenselaunchers,
    			  ship->missilestate == MISSILE_DAMAGED ? "Damaged" : "Good");
    ch_printf( ch, "\n&OMaxMods Ext:&Y %d&O/%d  Int: &Y%d&O/%d\n", 
            get_extmodule_count(ship), ship->maxextmodules, get_intmodule_count(ship), ship->maxintmodules );

};

static inline int compute_autofly_laser_ion_hit_chance( int chance, int origchance, SHIP_DATA *ship, SHIP_DATA *target )
{
    int distance;
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

    return chance;
}

static inline int compute_autopilot_projectile_launch_chance( int chance, SHIP_DATA *ship, SHIP_DATA *target )
{
    chance -= target->mod->manuever/5;
    chance -= target->currspeed/20;
    chance += target->shipclass*target->shipclass*25;
    chance -= ( abs( (int) ( target->vx - ship->vx ))/100 );
    chance -= ( abs( (int) ( target->vy - ship->vy ))/100 );
    chance -= ( abs( (int) ( target->vz - ship->vz ))/100 );
    chance += 30;
    chance = URANGE( 10 , chance , 90 );
    return chance;
}

static inline bool ship_target_in_combat_range( SHIP_DATA *ship, SHIP_DATA *target, int max_delta )
{
    if ( !target )
    return false;

    return ship_in_range( ship, target )
        && space_distance_ship_less_than( ship, target, max_delta );
}    

static inline int select_autopilot_projectile_type( SHIP_DATA *ship, SHIP_DATA *target )
{
    if( ( target->shipclass == SHIP_PLATFORM
       || ( target->shipclass == CAPITAL_SHIP && target->currspeed < 50 ) )
       && ship->rockets > 0 )
        return HEAVY_ROCKET;

    if( ( target->shipclass == MIDSIZE_SHIP || target->shipclass == CAPITAL_SHIP )
       && ship->torpedos > 0 )
        return PROTON_TORPEDO;

    if( ship->missiles < 0 && ship->torpedos > 0 )
        return PROTON_TORPEDO;

    if( ship->missiles < 0 && ship->rockets > 0 )
        return HEAVY_ROCKET;

    if( ship->missiles > 0 )
        return CONCUSSION_MISSILE;

    return -1;
}

static inline bool space_ship_is_not_docked( SHIP_DATA *ship )
{
    return ship && ship->docked == NULL && ship->shipstate != SHIP_DOCKED;
}

inline bool space_ship_is_ready( SHIP_DATA *ship )
{
    if (!ship)
        return false;
    return ship->shipstate == SHIP_READY;
}

inline bool space_validate_dock_links( CHAR_DATA *ch, SHIP_DATA *ship, bool showmsg = true )
{
    bool ok = true;

    if ( !ship )
        return false;

    if ( ship->docked == ship )
    {
        ship->docked = NULL;
        ship->docking = SHIP_READY;
        ok = false;
    }

    if ( ship->docking == SHIP_DOCKED && ship->docked == NULL )
    {
        ship->docking = SHIP_READY;
        ok = false;
    }

    if ( ship->docking == SHIP_READY && ship->docked != NULL )
    {
        ship->docked = NULL;
        ok = false;
    }

    if ( !ok && showmsg )
        send_to_char("&RDocking link desync detected. Control link reset.\n", ch);

    return ok;
}

inline bool space_validate_ship_links( CHAR_DATA *ch, SHIP_DATA *ship, bool showmsg = true )
{
    bool dock_ok = space_validate_dock_links( ch, ship, false );
    bool tractor_ok = space_validate_tractor_links( ch, ship, false );

    if ( !( dock_ok && tractor_ok ) && showmsg )
        send_to_char("&RShip control links desynced. Safety reset applied.\n", ch);

    return dock_ok && tractor_ok;
}

inline void space_release_tractor_target( SHIP_DATA *ship )
{
    SHIP_DATA *target;

    if ( !ship || !ship->tractored )
        return;

    target = ship->tractored;
    ship->tractored = NULL;

    if ( target->tractoredby == ship )
        target->tractoredby = NULL;

    if ( target->location )
        target->shipstate = SHIP_LANDED;
    else if ( target->shipstate != SHIP_DOCKED && target->shipstate != SHIP_DISABLED )
        target->shipstate = SHIP_READY;
}

inline bool space_enforce_ship_invariants( SHIP_DATA *ship, bool checkch = false )
{
    int old_shipstate;
    int old_docking;
    CHAR_DATA *old_ch;

    if ( !ship )
        return false;

    old_shipstate = ship->shipstate;
    old_docking = ship->docking;
    old_ch = ship->ch;

    space_validate_ship_links( NULL, ship, false );

    if ( ship->shipstate == SHIP_DOCKED && ship->docked == NULL )
        ship->shipstate = ship->location ? SHIP_LANDED : SHIP_READY;

    if ( ( ship->docking == SHIP_DOCK || ship->docking == SHIP_DOCK_2 ) && ship->docked == NULL )
    {
        ship->docking = SHIP_READY;
        if ( checkch )
            ship->ch = NULL;
    }

    if ( checkch
      && ( old_docking == SHIP_DOCK || old_docking == SHIP_DOCK_2 || old_docking == SHIP_DOCKED )
      && ship->docking == SHIP_READY
      && ship->docked == NULL )
        ship->ch = NULL;

    if ( ( ship->shipstate == SHIP_LAND || ship->shipstate == SHIP_LAND_2 )
      && ( !ship->dest || ship->dest[0] == '\0' ) )
        ship->shipstate = SHIP_READY;

    return old_shipstate != ship->shipstate
        || old_docking != ship->docking
        || old_ch != ship->ch;
}

inline void space_set_shipstate( SHIP_DATA *ship, int state )
{
    if ( !ship )
        return;

    ship->shipstate = state;
    space_enforce_ship_invariants( ship );
    ship->dirty = true;
}

inline void space_set_docking_state( SHIP_DATA *ship, int state )
{
    if ( !ship )
        return;

    ship->docking = state;
    space_enforce_ship_invariants( ship );
    ship->dirty = true;

}

inline void space_beg_docking_state( CHAR_DATA *ch, SHIP_DATA *ship, SHIP_DATA *dockto )
{
    ship->ch = ch;
    ship->docked = dockto;
    space_set_docking_state( ship, SHIP_DOCK );
}


inline void space_end_docking_state( SHIP_DATA *ship )
{
    ship->ch = NULL;
    ship->docked = NULL;    
    space_set_docking_state( ship, SHIP_READY );
}

void space_ship_force_docked_match (SHIP_DATA *ship, SHIP_DATA *dockedto )
{
            ship->vx = dockedto->vx;
            ship->vy = dockedto->vy;
            ship->vz = dockedto->vz;
            ship->cx = dockedto->cx;
            ship->cy = dockedto->cy;
            ship->cz = dockedto->cz;
            ship->ox = dockedto->ox;
            ship->oy = dockedto->oy;
            ship->oz = dockedto->oz;
            ship->jx = dockedto->jx;
            ship->jy = dockedto->jy;
            ship->jz = dockedto->jz;
            ship->hx = dockedto->hx;
            ship->hy = dockedto->hy;
            ship->hz = dockedto->hz;
            ship->hyperdistance = dockedto->hyperdistance;
            ship->currspeed = dockedto->currspeed;
            ship->orighyperdistance = dockedto->orighyperdistance;
            ship->location = dockedto->location;
            if ( dockedto->dest )
            {
                if ( !ship->dest || str_cmp( ship->dest, dockedto->dest ) )
                {
                    if ( ship->dest )
                        STRFREE( ship->dest );
                    ship->dest = STRALLOC( dockedto->dest );
                }
            }
            else if ( ship->dest )
            {
                STRFREE( ship->dest );
                ship->dest = NULL;
            }
            space_set_shipstate( ship, dockedto->shipstate );
            ship->spaceobject = dockedto->spaceobject;
            ship->currjump = dockedto->currjump;    
}