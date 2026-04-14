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
*			 Shop and repair shop module			   *
****************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdarg.h>
#include <sys/stat.h>
#include "mud.h"

#define VENDOR_FEE  .05 /*fee vendor charges, taken out of all goods with teh getcredits command*/

#define COST_EQUATION  (int) (cost*cost_equation( obj ))

float cost_equation( OBJ_DATA *obj )
{
      float count = obj->pIndexData->count;

	count = URANGE( 50, count, 500 );
      
	return (100/(count));
}



#if defined(KEY)
#undef KEY
#endif

#define KEY( literal, field, value )					\
				if ( !strcmp( word, literal ) )	\
				{					\
				    field  = value;			\
				    fMatch = TRUE;			\
				    break;				\
				}

std::string strip_aoran( const std::string &input );				

/*
 * Local functions
 */

#define	CD	CHAR_DATA
CD *	find_keeper	args( ( CHAR_DATA *ch ) );
CD *	find_keeper_q	args( ( CHAR_DATA *ch, bool message ) );
CD *	find_fixer	args( ( CHAR_DATA *ch ) );
int	get_cost	args( ( CHAR_DATA *ch, CHAR_DATA *keeper,
				OBJ_DATA *obj, bool fBuy ) );
int 	get_repaircost  args( ( CHAR_DATA *keeper, OBJ_DATA *obj ) );
#undef CD

const flag_name shop_types[] = {
    { SHOP_BUY, "buy" },
    { SHOP_FIX, "fix" },
    { SHOP_RECHARGE, "recharge" },
    { SHOP_MAX, "" },
    { (size_t)-1, nullptr } // terminator
};

int get_shoptype( const std::string& type )
{
	const flag_name* x;
	x = find_flag(shop_types, type);
	if (!x)
		return -1;
	return x->bit;
}

/*
 * Shopping commands.
 */
CHAR_DATA *find_keeper( CHAR_DATA *ch )
{
  return find_keeper_q( ch, TRUE );
}

CHAR_DATA *find_keeper_q( CHAR_DATA *ch, bool message )
{
    CHAR_DATA *keeper;
    SHOP_DATA *pShop;

    pShop = NULL;
    for ( keeper = ch->in_room->first_person;
	  keeper;
	  keeper = keeper->next_in_room )
	if ( IS_NPC(keeper) && (pShop = keeper->pIndexData->pShop) != NULL )
	    break;

    if ( !pShop )

    {
      if ( message )
	send_to_char( "You can't do that here.\n", ch );
      return NULL;
    }


    /*
     * Shop hours.
     */
    if ( pShop->open_hour > pShop->close_hour )
    {
      if( time_info.hour < pShop->open_hour && time_info.hour > pShop->close_hour )
      {
	do_say( keeper, "Sorry, come back later." );
	return NULL;
      }
    }
    else
      if( time_info.hour < pShop->open_hour || time_info.hour > pShop->close_hour )
      {
        if( time_info.hour > pShop->open_hour )
        {
	  do_say( keeper, "Sorry, come back later." );
	  return NULL;
        }
        if ( time_info.hour > pShop->close_hour )
        {
	  do_say( keeper, "Sorry, come back tomorrow." );
	  return NULL;
        }
      }
    if ( !knows_language( keeper, ch->speaking, ch ) )
    {
	do_say( keeper, "I can't understand you." );
	return NULL;
    }

    return keeper;
}

/*
 * repair commands.
 */
CHAR_DATA *find_fixer( CHAR_DATA *ch )
{
    CHAR_DATA *keeper;
    SHOP_DATA *rShop;

    rShop = NULL;
    for ( keeper = ch->in_room->first_person; keeper; keeper = keeper->next_in_room )
		if ( IS_NPC(keeper) && (rShop = keeper->pIndexData->pShop) != NULL && BV_IS_SET(rShop->shop_type, SHOP_FIX) )
		    break;

    if ( !rShop )
    {
	send_to_char( "You can't do that here.\n", ch );
	return NULL;
    }

 
    /*
     * Shop hours.
     */
    if ( time_info.hour < rShop->open_hour )
    {
	do_say( keeper, "Sorry, come back later." );
	return NULL;
    }

    if ( time_info.hour > rShop->close_hour )
    {
	do_say( keeper, "Sorry, come back tomorrow." );
	return NULL;
    }

    
    if ( !knows_language( keeper, ch->speaking, ch ) )
    {
	do_say( keeper, "I can't understand you." );
	return NULL;
    }

    return keeper;
}

int get_cost_quit( CHAR_DATA *ch )
{
	
  long cost = 1000;
  int golddem = 100000;
  long gold;
  
  if( !ch )
    return 0;
  
  if( ch->top_level <= 6 )
    return 0;
  
  gold = ch->gold + (IS_NPC(ch) ? 0 : ch->pcdata->bank) + 1;
  
  if( gold < 5000 )
    return 0;
  cost *= gold/golddem;

  return (int) cost;
}

int get_cost( CHAR_DATA *ch, CHAR_DATA *keeper, OBJ_DATA *obj, bool fBuy )
{
    SHOP_DATA *pShop;
    int cost;
    bool richcustomer;
    int profitmod;

    if ( !obj || ( pShop = keeper->pIndexData->pShop ) == NULL )
	return 0;

    if( obj->item_type == ITEM_FIGHTERCOMP || obj->item_type == ITEM_MIDCOMP || obj->item_type == ITEM_CAPITALCOMP )
    {
      int costunit;
      
      switch ( obj->value[1] )
      {
      	case MOD_HYPERSPEED: costunit = 1000; break;
      	case MOD_REALSPEED: costunit = 500; break;
      	case MOD_LASER: costunit = 10000; break;
      	case MOD_ION: costunit = 12500; break;
      	case MOD_MAXSHIELD: costunit = 1000; break;
      	case MOD_ENERGY: costunit = 10; break;
      	case MOD_LAUNCHER: costunit = 20000; break;
      	case MOD_TRACTORBEAM: costunit = 2500; break;
      	case MOD_COMM: costunit = 75; break;
      	case MOD_SENSOR: costunit = 125; break;
      	case MOD_ASTRO_ARRAY: costunit = 125; break;
      	case MOD_DEFENSELAUNCHER: costunit = 10000; break;
      	case MOD_MANUEVER: costunit = 1000; break;
      	case MOD_HULL: costunit = 200; break;
      	case MOD_MISSILE: costunit = 1000; break;
      	case MOD_TORPEDO: costunit = 1600; break;
      	case MOD_ROCKET: costunit = 2000; break;
      	case MOD_CHAFF: costunit = 400; break;
      	case MOD_GRAVITY_PROJ: costunit = 1000; break;
      	case MOD_TURRET: costunit = 100000; break;
      	case MOD_CARGO: costunit = 1; break;
      	default: costunit = 100000; break;
      }
      
      if( obj->item_type == ITEM_CAPITALCOMP )
      {
      	if ( obj->value[1] == MOD_MAXSHIELD )
      	  costunit *= 5;
        costunit *= 2;
      }
        
      cost = obj->value[3] * costunit;
      if( !(obj->value[3]) )
        cost += obj->value[2] * costunit;
	            	
      return cost;
      
    }

    cost = obj->cost;

    if ( ( ch->gold + (IS_NPC(ch) ? 0 : ch->pcdata->bank) ) > (ch->top_level * 1000) )
	richcustomer = TRUE;
    else
	richcustomer = FALSE;

    if ( fBuy )
    {
    	    cost = (int) (cost * (80 + UMIN(ch->top_level, LEVEL_AVATAR))) / 100;

    	    profitmod = 13 - get_curr_cha(ch) + (richcustomer ? 15 : 0)
	    	      + ((URANGE(5,ch->top_level,LEVEL_AVATAR)-20)/2);
	        cost = (int) (obj->cost
	             * UMAX( (pShop->profit_sell+1), pShop->profit_buy+profitmod ) )
	            / 100;
    } else {
	    OBJ_DATA *obj2;

	    profitmod = get_curr_cha(ch) - 13 - (richcustomer ? 15 : 0);
	    cost = 0;
			if ( BV_IS_SET(pShop->buy_type, obj->item_type) )
	        {
		            cost = (int) (obj->cost
		            * UMIN( (pShop->profit_buy-1),
			        pShop->profit_sell+profitmod) ) / 100;
	        }

            for ( obj2 = keeper->first_carrying; obj2; obj2 = obj2->next_content )
    	    {
	            if ( obj->pIndexData == obj2->pIndexData )
    	        {
    		        if( obj->item_type != ITEM_SPICE )
    		          cost /= (obj2->count+1);
	    	        break;
	            }
	        }
    
            cost = UMIN( cost , 2500 );
    }


    if (obj->item_type != ITEM_CARGO && obj->item_type != ITEM_SPICE ) {
        if( cost > 0 )
        {
            cost = COST_EQUATION;
            if( cost <= 0 )
            cost = 1;
        }
    }
      
    if ( obj->item_type == ITEM_ARMOR )
      cost = (int) (cost * (obj->value[0]+1) / (obj->value[1]+1) );
    if ( obj->item_type == ITEM_WEAPON )
    {
      cost = (int) (cost * (obj->value[0]+1) / INIT_WEAPON_CONDITION+1);
      cost = (int) (cost * (obj->value[4]+1) / (obj->value[5]+1));
    }
    	
    if ( obj->item_type == ITEM_DEVICE )
	cost = (int) (cost * (obj->value[2]+1) / (obj->value[1]+1));

    return cost;
}

int get_repaircost( CHAR_DATA *keeper, OBJ_DATA *obj )
{
    REPAIR_DATA *rShop;
    int cost;
    bool found;

    if ( !obj || ( rShop = keeper->pIndexData->rShop ) == NULL )
	return 0;

    cost = 0;
    found = FALSE;
	if ( BV_IS_SET(rShop->fix_type, obj->item_type) )
	{
	    cost = (int) (obj->cost * rShop->profit_fix / 100);
	    found = TRUE;
	}

    if ( !found )
      cost = -1;

    if ( cost == 0 )
      cost = 1;

    if ( found && cost > 0 )
    {
      switch (obj->item_type)
      {
	case ITEM_ARMOR:
	  if (obj->value[0] >= obj->value[1])
	    cost = -2;
	  else
	    cost *= (obj->value[1] - obj->value[0]);
	  break;
	case ITEM_WEAPON:
	  if (INIT_WEAPON_CONDITION == obj->value[0])
	    cost = -2;
	  else
	    cost *= (INIT_WEAPON_CONDITION - obj->value[0]);
	  break;
	case ITEM_DEVICE:
	  if (obj->value[2] >= obj->value[1])
	    cost = -2;
	  else
	    cost *= (obj->value[1] - obj->value[2]);
      }
    }

    return cost;
}



void do_buy( CHAR_DATA *ch, char *argument )
{
    std::string arg;
	std::string argstr = argument;
    int maxgold;

    argstr = one_argument( argstr, arg );

    if ( arg.empty() )
    {
	send_to_char( "Buy what?\n", ch );
	return;
    }

    if ( BV_IS_SET(ch->in_room->room_flags, ROOM_PET_SHOP) )
    {
	std::string buf;
	CHAR_DATA *pet;
	ROOM_INDEX_DATA *pRoomIndexNext;
	ROOM_INDEX_DATA *in_room;

	if ( IS_NPC(ch) )
	    return;

	pRoomIndexNext = get_room_index( ch->in_room->vnum + 1 );
	if ( !pRoomIndexNext )
	{
	    bug( "Do_buy: bad pet shop at vnum %d.", ch->in_room->vnum );
	    send_to_char( "Sorry, you can't buy that here.\n", ch );
	    return;
	}

	in_room     = ch->in_room;
	ch->in_room = pRoomIndexNext;
	pet         = get_char_room( ch, arg );
	ch->in_room = in_room;

	if ( pet == NULL || !IS_NPC( pet ) || !BV_IS_SET(pet->act, ACT_PET) )
	{
	    send_to_char( "Sorry, you can't buy that here.\n", ch );
	    return;
	}

	if ( BV_IS_SET(ch->act, PLR_BOUGHT_PET) )
	{
	    send_to_char( "You already bought one pet this level.\n", ch );
	    return;
	}

	if ( ch->gold < 10 * pet->top_level * pet->top_level )
	{
	    send_to_char( "You can't afford it.\n", ch );
	    return;
	}

	if ( ch->top_level < pet->top_level )
	{
	    send_to_char( "You're not ready for this pet.\n", ch );
	    return;
	}

	maxgold = 10 * pet->top_level * pet->top_level;
	ch->gold	-= maxgold;
	boost_economy( ch->in_room->area, maxgold );

	if (!ch->game)
	{
    	send_to_char( "You must be in a game to buy a pet.\n", ch );
	    return;
	}
	
	pet		= create_mobile( ch->game, pet->pIndexData );
//	BV_SET_BIT(ch->act, PLR_BOUGHT_PET);
	BV_SET_BIT(pet->act, ACT_PET);
	BV_SET_BIT(pet->affected_by, AFF_CHARM);

	argstr = one_argument( argstr, arg );
	if ( !arg.empty() )
	{
	    buf = str_printf( "%s %s", pet->name, arg );
	    STRFREE( pet->name );
	    pet->name = STRALLOC( buf );
	}

	buf = str_printf( "%sA neck tag says 'I belong to %s'.\n",
	    pet->description, ch->name );
	STRFREE( pet->description );
	pet->description = STRALLOC( buf );

	if( ch->pcdata )
	  ch->pcdata->pet = pet;

	char_to_room( pet, ch->in_room );
	add_follower( pet, ch );
	send_to_char( "Enjoy your pet.\n", ch );
    	act( AT_ACTION, "$n bought $N as a pet.", ch, NULL, pet, TO_ROOM );
	return;
    }
    else
    {
	CHAR_DATA *keeper;
	OBJ_DATA *obj;
	int cost;
	int noi = 1;		/* Number of items */
	sh_int mnoi = 20;	/* Max number of items to be bought at once */

	if ( ( keeper = find_keeper( ch ) ) == NULL )
	    return;

	if ( keeper == NULL )
	  return;

	maxgold = keeper->top_level * 10;

	if ( is_number( arg ) )
	{
	    noi = strtoi( arg );
	    argstr = one_argument( argstr, arg );
	    if ( noi > mnoi )
	    {
		act( AT_TELL, "$n tells you 'I don't sell that many items at"
		  " once.'", keeper, NULL, ch, TO_VICT );
		ch->reply = keeper;
		return;
	    }
	}

	obj  = get_obj_carry( keeper, arg );
	
	if ( !obj && !arg.empty() && arg[0] == '#' )
        {     
              int onum, oref;
              bool ofound = FALSE;
              
              onum =0;
              oref = strtoi(arg.c_str()+1);
              for ( obj = keeper->last_carrying; obj; obj = obj->prev_content )
	      { 
	        if ( obj->wear_loc == WEAR_NONE
	        &&   can_see_obj( ch, obj ) )
	            onum++;
                if ( onum == oref ) 
                {
                    ofound = TRUE;
                    break;
                }
                else if ( onum > oref )
                   break;
	      }
	      if (!ofound)
	         obj = NULL;
        }

        if ( !obj )
        {
	  send_to_char( "Buy what?\n", ch );
	  return;
        }
          
	
	cost = ( get_cost( ch, keeper, obj, TRUE ) * noi );

        if (keeper->home != NULL && obj->cost > 0)
          cost= obj->cost;
	
	
	if ( cost <= 0 || !can_see_obj( ch, obj ) )
	{
	    act( AT_TELL, "$n tells you 'I don't sell that -- try 'list'.'",
		keeper, NULL, ch, TO_VICT );
	    ch->reply = keeper;
	    return;
	}

	if ( !IS_OBJ_STAT( obj, ITEM_INVENTORY ) && ( noi > 1 ) )
	{
	    interpret( keeper, "laugh" );
	    act( AT_TELL, "$n tells you 'I don't have enough of those in stock"
	     " to sell more than one at a time.'", keeper, NULL, ch, TO_VICT );
	    ch->reply = keeper;
	    return;
	}

    /* Check local supply for availability
     * -Aran
     */
    if( obj->item_type == ITEM_CARGO ) {
        /* Cargo type */
        int v0 = obj->value[0];
            
        /* Look at 10% of daily net produce */
        int consume = 36 * ch->in_room->area->depletion[v0];
        int produce = 36 * ch->in_room->area->production[v0];

        /* Change in supply over the next 10% of the day */
        int supply = produce - consume;

        /* Change in suplpy if we sell the cargo */
        supply = supply - obj->value[1];

        /* Don't sell if change drops area to less than 75% of current */
        if ((ch->in_room->area->supply[v0] + supply) <
                (int)(0.75f * (float)ch->in_room->area->supply[v0])) {
            interpret( keeper, "mutter" );
            act( AT_TELL, "$n tells you 'Our local supply is running low. "
                          "Come back when we have more to sell.'",
                          keeper, NULL, ch, TO_VICT);
            ch->reply = keeper;
            return;
        }
    }

    
	if ( ch->gold < cost )
	{
	    act( AT_TELL, "$n tells you 'You can't afford to buy $p.'",
		keeper, obj, ch, TO_VICT );
	    ch->reply = keeper;
	    return;
	}

	if ( BV_IS_SET(obj->objflags, ITEM_PROTOTYPE) 
             && get_trust( ch ) < LEVEL_IMMORTAL )
	{
	    act( AT_TELL, "$n tells you 'This is a only a prototype!  I can't sell you that...'", 
		keeper, NULL, ch, TO_VICT );
      	    ch->reply = keeper;
	    return;
	}

	if ( ch->carry_number + get_obj_number( obj ) > can_carry_n( ch ) )
	{
	    send_to_char( "You can't carry that many items.\n", ch );
	    return;
	}

	if ( ch->carry_weight + ( get_obj_weight( obj ) * noi )
		+ (noi > 1 ? 2 : 0) > can_carry_w( ch ) )
	{
	    send_to_char( "You can't carry that much weight.\n", ch );
	    return;
	}

        separate_obj( obj );

	if ( noi == 1 )
	{
//	    if ( !IS_OBJ_STAT( obj, ITEM_INVENTORY ) || ( keeper->home != NULL ) )  
//            separate_obj( obj );
	    act( AT_ACTION, "$n buys $p.", ch, obj, NULL, TO_ROOM );
    	    act( AT_ACTION, "You buy $p.", ch, obj, NULL, TO_CHAR );
	}
        else
	{
		std::string desc = strip_aoran( obj->short_descr );
		arg = str_printf( "$n buys %d %s%s.", noi,
			desc.c_str(), ( !desc.empty() && desc.back() == 's' ? "" : "s" ) );
	    act( AT_ACTION, arg.c_str(), ch, obj, NULL, TO_ROOM );
	    arg = str_printf( "You buy %d %s%s.", noi,
		desc.c_str(), ( !desc.empty() && desc.back() == 's' ? "" : "s" ) );
	    act( AT_ACTION, arg.c_str(), ch, obj, NULL, TO_CHAR );
	    act( AT_ACTION, "$N puts them into a bag and hands it to you.",
		ch, NULL, keeper, TO_CHAR );
	}

	ch->gold     -= cost;
	keeper->gold += cost;

	if ( ( keeper->gold > maxgold ) && (keeper->owner == NULL ))
	{
	    boost_economy( keeper->in_room->area, keeper->gold - maxgold/2 );
	    keeper->gold = maxgold/2;
	    act( AT_ACTION, "$n puts some credits into a large safe.", keeper, NULL, NULL, TO_ROOM );
	}
	if( obj->item_type == ITEM_CARGO )
		ch->in_room->area->supply[obj->value[0]] -= obj->value[1]*noi;

	if ( IS_OBJ_STAT( obj, ITEM_INVENTORY ) && ( keeper->home == NULL ) )
	{
	    OBJ_DATA *buy_obj, *bag;

	    buy_obj = create_object( obj->pIndexData, obj->level );
	    if ( obj->count > 1 )
	    {
	      obj->pIndexData->count -= ( obj->count - 1 );
	      obj->count = 1;
	    }

	    /*
	     * Due to grouped objects and carry limitations in SMAUG
	     * The shopkeeper gives you a bag with multiple-buy,
	     * and also, only one object needs be created with a count
	     * set to the number bought.		-Thoric
	     */
	    if ( noi > 1 )
	    {
		bag = create_object( get_obj_index( OBJ_VNUM_SHOPPING_BAG ), 1 );
		/* perfect size bag ;) */
		bag->value[0] = bag->weight + (buy_obj->weight * noi);
		buy_obj->count = noi;
		obj->pIndexData->count += (noi - 1);
		numobjsloaded += (noi - 1);
		obj_to_obj( buy_obj, bag );
		obj_to_char( bag, ch );

                /* vendor snippit. Forces vendor to save after anyone buys anything*/
 		if (  keeper->home != NULL )
 		{
		  save_vendor (keeper);
		  bag->cost = 0;
		}
	    }
	    else
			obj_to_char( buy_obj, ch );

		/* vendor snippit. Forces vendor to save after anyone buys anything*/
 		if (  keeper->home != NULL )
 		{
		  save_vendor (keeper);
		  buy_obj->cost = 0;
		}
	}
        else
	{
	    separate_obj( obj );
	    
	    obj_from_char( obj );
	    obj_to_char( obj, ch );

	    /* vendor snippit. Forces vendor to save after anyone buys anything*/
 		if (  keeper->home != NULL )
 		{
		  save_vendor (keeper);
		  obj->cost = 0;
		}
	}

	return;
    }
}


void do_list( CHAR_DATA *ch, char *argument )
{
    if ( BV_IS_SET(ch->in_room->room_flags, ROOM_PET_SHOP) )
    {
	ROOM_INDEX_DATA *pRoomIndexNext;
	CHAR_DATA *pet;
	bool found;

	pRoomIndexNext = get_room_index( ch->in_room->vnum + 1 );
	if ( !pRoomIndexNext )
	{
	    bug( "Do_list: bad pet shop at vnum %d.", ch->in_room->vnum );
	    send_to_char( "You can't do that here.\n", ch );
	    return;
	}

	found = FALSE;
	for ( pet = pRoomIndexNext->first_person; pet; pet = pet->next_in_room )
	{
	    if ( BV_IS_SET(pet->act, ACT_PET) && IS_NPC(pet) )
	    {
		if ( !found )
		{
		    found = TRUE;
		    send_to_char( "Pets for sale:\n", ch );
		}
		ch_printf( ch, "[%2d] %8d - %s\n",
			pet->top_level,
			10 * pet->top_level * pet->top_level,
			pet->short_descr );
	    }
	}
	if ( !found )
	    send_to_char( "Sorry, we're out of pets right now.\n", ch );
	return;
    }
    else
    {
		std::string arg;
		CHAR_DATA *keeper;
		OBJ_DATA *obj;
		int cost;
		int oref = 0;
		bool found;

		one_argument( argument, arg );

		if ( ( keeper = find_keeper( ch ) ) == NULL )
			return;

		found = FALSE;
		for ( obj = keeper->last_carrying; obj; obj = obj->prev_content )
		{
			if ( obj->wear_loc == WEAR_NONE
			&&   can_see_obj( ch, obj ) )
			{
			oref++;
			if ( ( cost = get_cost( ch, keeper, obj, TRUE ) ) > 0
			&& ( arg.empty() || nifty_is_name_prefix( arg, obj->name ) ) )
			{
			if (keeper->home != NULL)
			cost = obj->cost;
			if ( !found )
			{
				found = TRUE;
				send_to_char( "[Price] {ref} Item\n", ch );
			}
			if ( obj->item_type == ITEM_FIGHTERCOMP )
			{
			ch_printf( ch, "[%5d] {%3d} %s (Fighter)\n                %s Mod: %d Size: %d Cond: %d%%.\n",
			cost, oref, capitalize( obj->short_descr ).c_str(),
			show_mod_type2( obj->value[1] ),
			obj->value[3], obj->value[2], obj->value[0] );
			continue;
			}

			if ( obj->item_type == ITEM_MIDCOMP )
			{
			ch_printf( ch, "[%5d] {%3d} %s (Midsize)\n                %s Mod: %d Size: %d Cond: %d%%.\n",
			cost, oref, capitalize( obj->short_descr ).c_str(),
			show_mod_type2( obj->value[1] ),
			obj->value[3], obj->value[2], obj->value[0] );
			continue;
			}

			if ( obj->item_type == ITEM_CAPITALCOMP )
			{
			ch_printf( ch, "[%5d] {%3d} %s (Capital)\n                %s Mod: %d Size: %d Cond: %d%%.\n",
			cost, oref, capitalize( obj->short_descr ).c_str(),
			show_mod_type2( obj->value[1] ),
			obj->value[3], obj->value[2], obj->value[0] );
			continue;
			}

			ch_printf( ch, "[%5d] {%3d} %s%s.\n",
			cost, oref, capitalize( obj->short_descr ).c_str(),
				BV_IS_SET(obj->objflags, ITEM_HUTT_SIZE) ? " (hutt size)" :
				( BV_IS_SET(obj->objflags, ITEM_LARGE_SIZE) ? " (large)" :
				( BV_IS_SET(obj->objflags, ITEM_HUMAN_SIZE) ? " (medium)" :
				( BV_IS_SET(obj->objflags, ITEM_SMALL_SIZE) ? " (small)" :
				"" ) ) ) );
			}
			}
		}

		if ( !found )
		{
			if ( arg.empty() )
			send_to_char( "You can't buy anything here.\n", ch );
			else
			send_to_char( "You can't buy that here.\n", ch );
		}
		return;
    }
}


void do_sell( CHAR_DATA *ch, char *argument )
{
    std::string buf;
    std::string arg;
    CHAR_DATA *keeper;
    OBJ_DATA *obj;
    int cost;
    bool spice = FALSE;

    one_argument( argument, arg );

    if ( arg.empty() )
    {
	send_to_char( "Sell what?\n", ch );
	return;
    }

    if ( ( keeper = find_keeper( ch ) ) == NULL )
	return;

    if ( ( obj = get_obj_carry( ch, arg ) ) == NULL )
    {
	act( AT_TELL, "$n tells you 'You don't have that item.'",
		keeper, NULL, ch, TO_VICT );
	ch->reply = keeper;
	return;
    }

    if ( !can_drop_obj( ch, obj ) )
    {
	send_to_char( "You can't let go of it!\n", ch );
	return;
    }

    if ( obj->timer > 0 )
    {
	act( AT_TELL, "$n tells you, '$p is depreciating in value too quickly...'", keeper, obj, ch, TO_VICT );
	return;
    }

    if ( ( cost = get_cost( ch, keeper, obj, FALSE ) ) <= 0 )
    {
	act( AT_ACTION, "$n looks uninterested in $p.", keeper, obj, ch, TO_VICT );
	return;
    }

    if( obj->item_type == ITEM_SPICE )
      spice = TRUE;

    if ( cost > keeper->gold && ( economy_has( ch->in_room->area, cost) || spice ) )
    {
	act( AT_TELL, "$n makes a credit transaction.", keeper, obj, ch, TO_VICT );
        lower_economy( ch->in_room->area, cost-keeper->gold );
    }
    if ( !economy_has( ch->in_room->area, cost ) && !spice )
    {
	act( AT_ACTION, "$n can not afford $p right now.", keeper, obj, ch, TO_VICT );
	return;
    }
   
    separate_obj( obj );
    act( AT_ACTION, "$n sells $p.", ch, obj, NULL, TO_ROOM );
    buf = str_printf( "You sell $p for %d credit%s.",
	cost, cost == 1 ? "" : "s" );
    act( AT_ACTION, buf.c_str(), ch, obj, NULL, TO_CHAR );
    ch->gold     += cost;
    keeper->gold -= cost;
    if ( spice )
      boost_economy( ch->in_room->area, (int) ( cost*1.5 ) );
    if ( keeper->gold < 0 )
	keeper->gold = 0;

    /* Modify area supply after selling cargo
     * -Aran
     */
    if( obj->item_type == ITEM_CARGO ) {
            int v0 = obj->value[0];
            ch->in_room->area->supply[v0] += obj->value[1];
    }

    if ( obj->item_type == ITEM_TRASH )
	extract_obj( obj );
    else  if ( BV_IS_SET( obj->objflags , ITEM_CONTRABAND) )
   {
       long ch_exp;
       
       ch_exp = UMIN( obj->cost*10 , ( exp_level( ch->skill_level[SMUGGLING_ABILITY]+1) - exp_level( ch->skill_level[SMUGGLING_ABILITY])  ) / 10  );
       ch_printf( ch, "You receive %ld smuggling experience for unloading your contraband.\n " , ch_exp );
       gain_exp( ch, ch_exp , SMUGGLING_ABILITY );
       if ( obj->item_type == ITEM_SPICE || obj->item_type == ITEM_RAWSPICE )
	 extract_obj( obj );
       else
       {
         BV_REMOVE_BIT( obj->objflags , ITEM_CONTRABAND );
         obj_from_char( obj );
         obj_to_char( obj, keeper );
       }
   }
    else if ( obj->item_type == ITEM_SPICE || obj->item_type == ITEM_RAWSPICE )
	extract_obj( obj );
    else
    {
	obj_from_char( obj );
	obj_to_char( obj, keeper );
    }

    return;
}



void do_value( CHAR_DATA *ch, char *argument )
{
    std::string buf;
    CHAR_DATA *keeper;
    OBJ_DATA *obj;
    int cost;

    if ( argument[0] == '\0' )
    {
	send_to_char( "Value what?\n", ch );
	return;
    }

    if ( ( keeper = find_keeper( ch ) ) == NULL )
	return;

    if ( ( obj = get_obj_carry( ch, argument ) ) == NULL )
    {
	act( AT_TELL, "$n tells you 'You don't have that item.'",
		keeper, NULL, ch, TO_VICT );
	ch->reply = keeper;
	return;
    }

    if ( !can_drop_obj( ch, obj ) )
    {
	send_to_char( "You can't let go of it!\n", ch );
	return;
    }

    if ( ( cost = get_cost( ch, keeper, obj, FALSE ) ) <= 0 )
    {
	act( AT_ACTION, "$n looks uninterested in $p.", keeper, obj, ch, TO_VICT );
	return;
    }

    buf = str_printf( "$n tells you 'I'll give you %d credits for $p.'", cost );
    act( AT_TELL, buf.c_str(), keeper, obj, ch, TO_VICT );
    ch->reply = keeper;

    return;
}

/*
 * Repair a single object. Used when handling "repair all" - Gorog
 */
void repair_one_obj( CHAR_DATA *ch, CHAR_DATA *keeper, OBJ_DATA *obj,
                 char *arg, int maxgold )
{
   std::string buf;
   int cost;

   	if ( !can_drop_obj( ch, obj ) )
       	ch_printf( ch, "You can't let go of %s.\n", obj->name );
   	else if ( ( cost = get_repaircost( keeper, obj ) ) < 0 )
   	{
       	if (cost != -2)
       		act( AT_TELL, "$n tells you, 'Sorry, I can't do anything with $p.'", 
            	keeper, obj, ch, TO_VICT );
       	else
	  		act( AT_TELL, "$n tells you, '$p looks fine to me!'", keeper, obj, ch, TO_VICT );
   	}
               /* "repair all" gets a 10% surcharge - Gorog */

	else if ( (cost = strcmp("all",arg) ? cost : 11*cost/10) > ch->gold )
	{
		buf = str_printf(
		"$N tells you, 'It will cost %d credit%s to maintain %s...'", cost,
		cost == 1 ? "" : "s", obj->name );
		act( AT_TELL, buf, ch, NULL, keeper, TO_CHAR );
		act( AT_TELL, "$N tells you, 'Which I see you can't afford.'", ch,
				NULL, keeper, TO_CHAR );
	}
	else
	{
		buf = "$n gives $p to $N, who quickly maintains it.";
		act( AT_ACTION, buf.c_str(), ch, obj, keeper, TO_ROOM );
		buf = str_printf( "$N charges you %d credit%s to maintain $p.",
			cost, cost == 1 ? "" : "s" );
		act( AT_ACTION, buf.c_str(), ch, obj, keeper, TO_CHAR );
		ch->gold     -= cost;
		keeper->gold += cost;
		if ( keeper->gold < 0 )
			keeper->gold = 0;
		else
		if ( keeper->gold > maxgold )
		{
			boost_economy( keeper->in_room->area, keeper->gold - maxgold/2 );
			keeper->gold = maxgold/2;
			act( AT_ACTION, "$n puts some credits into a large safe.", keeper, 
		NULL, NULL, TO_ROOM );
		}

		switch ( obj->item_type )
		{
			default:
			send_to_char( "For some reason, you think you got ripped off...\n", ch);
			break;
			case ITEM_ARMOR:
			obj->value[0] = obj->value[1];
			break;
			case ITEM_WEAPON:
			obj->value[0] = INIT_WEAPON_CONDITION;
			break;
			case ITEM_DEVICE:
			obj->value[2] = obj->value[1];
			break;
		}

		oprog_repair_trigger( ch, obj );
	}
}

void do_repair( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *keeper;
    OBJ_DATA *obj;
    int maxgold;

    if ( argument[0] == '\0' )
    {
	send_to_char( "Repair what?\n", ch );
	return;
    }

    if ( ( keeper = find_fixer( ch ) ) == NULL )
	return;

    maxgold = keeper->top_level * 10;

/*
	switch( keeper->pIndexData->pShop->shop_type )
    {
	default:
	case SHOP_FIX:
	  fixstr  = "repair";
	  fixstr2 = "repairs";
	  break;
	case SHOP_RECHARGE:
	  fixstr  = "recharge";
	  fixstr2 = "recharges";
	  break;
    }
*/
    if ( !strcmp( argument, "all" ) )
    {
	for ( obj = ch->first_carrying; obj ; obj = obj->next_content )
	{
           if ( obj->wear_loc  == WEAR_NONE
           &&   can_see_obj( ch, obj )
	   && ( obj->item_type == ITEM_ARMOR
	   ||   obj->item_type == ITEM_WEAPON
	   ||   obj->item_type == ITEM_DEVICE ) )
                repair_one_obj( ch, keeper, obj, argument, maxgold);
	}
    return;
    }

    if ( ( obj = get_obj_carry( ch, argument ) ) == NULL )
    {
	act( AT_TELL, "$n tells you 'You don't have that item.'",
		keeper, NULL, ch, TO_VICT );
	ch->reply = keeper;
	return;
    }

    repair_one_obj( ch, keeper, obj, argument, maxgold); }

void appraise_all( CHAR_DATA *ch, CHAR_DATA *keeper, const std::string& fixstr )
{
    OBJ_DATA *obj;
    std::string buf;
    int cost, total=0;

    for ( obj = ch->first_carrying; obj != NULL ; obj = obj->next_content )
    {
        if ( obj->wear_loc  == WEAR_NONE
        &&   can_see_obj( ch, obj )
        && ( obj->item_type == ITEM_ARMOR
        ||   obj->item_type == ITEM_WEAPON
        ||   obj->item_type == ITEM_DEVICE ) )
        {

            if ( !can_drop_obj( ch, obj ) )
            ch_printf( ch, "You can't let go of %s.\n", obj->name );
            else if ( ( cost = get_repaircost( keeper, obj ) ) < 0 )
            {
               if (cost != -2)
               act( AT_TELL,
                    "$n tells you, 'Sorry, I can't do anything with $p.'",
                    keeper, obj, ch, TO_VICT );
               else
               act( AT_TELL, "$n tells you, '$p looks fine to me!'",
                    keeper, obj, ch, TO_VICT );
            }
            else 
            {
            buf = str_printf(
            "$N tells you, 'It will cost %d credit%s to %s %s'",
            cost, cost == 1 ? "" : "s", "maintain", obj->name );
            act( AT_TELL, buf, ch, NULL, keeper, TO_CHAR );
            total += cost;
            }
        }
    }
    if ( total > 0 )
    {
       send_to_char ("\n", ch);
       buf = str_printf(
          "$N tells you, 'It will cost %d credit%s in total.'",
          total, total == 1 ? "" : "s" );
       act( AT_TELL, buf, ch, NULL, keeper, TO_CHAR );
       buf = "$N tells you, 'Remember there is a 10%% surcharge for repair all.'";
       act( AT_TELL, buf, ch, NULL, keeper, TO_CHAR );
    }
}
	

void do_appraise( CHAR_DATA *ch, char *argument )
{
    std::string buf;
    std::string arg;
    CHAR_DATA *keeper;
    OBJ_DATA *obj;
    int cost;
    std::string fixstr;

    one_argument( argument, arg );

    if ( arg.empty() )
    {
		send_to_char( "Appraise what?\n", ch );
		return;
    }

    if ( ( keeper = find_fixer( ch ) ) == NULL )
		return;

    if ( !str_cmp( arg, "all") )
    {
		appraise_all( ch, keeper, fixstr );
		return;
    }

    if ( ( obj = get_obj_carry( ch, arg ) ) == NULL )
    {
	act( AT_TELL, "$n tells you 'You don't have that item.'",
		keeper, NULL, ch, TO_VICT );
	ch->reply = keeper;
	return;
    }

    if ( !can_drop_obj( ch, obj ) )
    {
	send_to_char( "You can't let go of it.\n", ch );
	return;
    }

    if ( ( cost = get_repaircost( keeper, obj ) ) < 0 )
    {
      if (cost != -2)
	act( AT_TELL, "$n tells you, 'Sorry, I can't do anything with $p.'", keeper, obj, ch, TO_VICT );
      else
	act( AT_TELL, "$n tells you, '$p looks fine to me!'", keeper, obj, ch, TO_VICT );
      return;
    }

    buf = str_printf(
       "$N tells you, 'It will cost %d credit%s to %s that...'", cost,
       cost == 1 ? "" : "s", fixstr.c_str() );
    act( AT_TELL, buf, ch, NULL, keeper, TO_CHAR );
    if ( cost > ch->gold )
      act( AT_TELL, "$N tells you, 'Which I see you can't afford.'", ch,
	 NULL, keeper, TO_CHAR );

    return;
}


/* ------------------ Shop Building and Editing Section ----------------- */


void do_makeshop( CHAR_DATA *ch, char *argument )
{
    SHOP_DATA *shop;
    int vnum;
    MOB_INDEX_DATA *mob;

    if ( !argument || argument[0] == '\0' )
    {
	send_to_char( "Usage: makeshop <mobvnum>\n", ch );
	return;
    }

    vnum = atoi( argument );
    
    if ( (mob = get_mob_index(vnum)) == NULL )
    {
	send_to_char( "Mobile not found.\n", ch );
	return;
    }

    if ( !can_medit(ch, mob) )
      return;

    if ( mob->pShop )
    {
	send_to_char( "This mobile already has a shop.\n", ch );
	return;
    }

    CREATE( shop, SHOP_DATA, 1 );
	shop->game = ch->game;
    LINK( shop, first_shop, last_shop, next, prev );
    shop->keeper	= vnum;
    shop->profit_buy	= 120;
    shop->profit_sell	= 90;
    shop->open_hour	= 0;
    shop->close_hour	= 23;
    mob->pShop		= shop;
    send_to_char( "Done.\n", ch );
    return;
}

void do_shopset_old( CHAR_DATA *ch, char *argument )
{
    SHOP_DATA *shop;
    MOB_INDEX_DATA *mob, *mob2;
    int vnum;
    int value;
    std::string arg1;
    std::string arg2;
	std::string argstr = argument;
    
    argstr = one_argument( argstr, arg1 );
    argstr = one_argument( argstr, arg2 );

    if ( arg1.empty() || arg2.empty() )
    {
	send_to_char( "Usage: shopset <mob vnum> <field> value\n", ch );
	send_to_char( "\nField being one of:\n", ch );
	send_to_char( "  shoptype itemtype buy sell open close keeper\n", ch );
	return;
    }

    vnum = strtoi( arg1 );

    if ( (mob = get_mob_index(vnum)) == NULL )
    {
	send_to_char( "Mobile not found.\n", ch );
	return;
    }

    if ( !can_medit(ch, mob) )
      return;

    if ( !mob->pShop )
    {
	send_to_char( "This mobile doesn't keep a shop.\n", ch );
	return;
    }
    shop = mob->pShop;
    value = strtoi( argstr );

	if ( !str_cmp( arg2, "itemtype" ))
	{
		int value = -1;
		value = get_otype( argstr );
		if ( value < 0 || value > ITEMTYPE_MAX )
		{
			send_to_char( "Invalid item type!\n", ch );
			return;
		}
		BV_TOGGLE_BIT(shop->buy_type, value);
		send_to_char( "Done.\n", ch );
		return;
	}

	if ( !str_cmp( arg2, "shoptype" ))
	{
		int value = -1;
		value = get_shoptype( argstr );
		if ( value < 0 || value > SHOP_MAX )
		{
			send_to_char( "Invalid shop type!\n", ch );
			return;
		}
		BV_TOGGLE_BIT(shop->shop_type, value);
		send_to_char( "Done.\n", ch );
		return;		
	}

     if ( !str_cmp( arg2, "buy" ) )
    {
		if ( value <= (shop->profit_sell) || value > 1000 )
		{
			send_to_char( "Out of range.\n", ch );
			return;
		}
		shop->profit_buy = value;
		send_to_char( "Done.\n", ch );
		return;
    }

    if ( !str_cmp( arg2, "sell" ) )
    {
	if ( value < 0 || value >= (shop->profit_buy) )
	{
	    send_to_char( "Out of range.\n", ch );
	    return;
	}
	shop->profit_sell = value;
	send_to_char( "Done.\n", ch );
	return;
    }

    if ( !str_cmp( arg2, "open" ) )
    {
	if ( value < 0 || value > 23 )
	{
	    send_to_char( "Out of range.\n", ch );
	    return;
	}
	shop->open_hour = value;
	send_to_char( "Done.\n", ch );
	return;
    }

    if ( !str_cmp( arg2, "close" ) )
    {
	if ( value < 0 || value > 23 )
	{
	    send_to_char( "Out of range.\n", ch );
	    return;
	}
	shop->close_hour = value;
	send_to_char( "Done.\n", ch );
	return;
    }

    if ( !str_cmp( arg2, "keeper" ) )
    {
	if ( (mob2 = get_mob_index(vnum)) == NULL )
	{
	    send_to_char( "Mobile not found.\n", ch );
	    return;
	}
	if ( !can_medit(ch, mob) )
	    return;
	if ( mob2->pShop )
	{
	    send_to_char( "That mobile already has a shop.\n", ch );
	    return;
	}
	mob->pShop  = NULL;
	mob2->pShop = shop;
	shop->keeper = value;
	send_to_char( "Done.\n", ch );
	return;
    }

    do_shopset( ch, "" );
    return;
}


void do_shopstat( CHAR_DATA *ch, char *argument )
{
    SHOP_DATA *shop;
    MOB_INDEX_DATA *mob;
    int vnum;
    
    if ( argument[0] == '\0' )
    {
	send_to_char( "Usage: shopstat <keeper vnum>\n", ch );
	return;
    }

    vnum = atoi( argument );
    
    if ( (mob = get_mob_index(vnum)) == NULL )
    {
	send_to_char( "Mobile not found.\n", ch );
	return;
    }

    if ( !mob->pShop )
    {
	send_to_char( "This mobile doesn't keep a shop.\n", ch );
	return;
    }
    shop = mob->pShop;

    ch_printf( ch, "Keeper: %d  %s\n", shop->keeper, mob->short_descr );
    ch_printf( ch, "Shops: %s\n", bitset_to_string(shop->shop_type, shop_types).c_str() );
    ch_printf( ch, "buytype: %s\n", bitset_to_string(shop->buy_type, o_types).c_str() );
    ch_printf( ch, "Profit:  buy %3d%%  sell %3d%%  fix %3d%%\n",
			shop->profit_buy,
			shop->profit_sell,
			shop->profit_fix );
    ch_printf( ch, "Hours:   open %2d  close %2d\n",
			shop->open_hour,
			shop->close_hour );
    return;
}

std::string shop_type_to_string( SHOP_DATA *shop )
{
	std::string result;
	if ( BV_IS_SET(shop->shop_type, SHOP_BUY) )
		result += "B";	
	if ( BV_IS_SET(shop->shop_type, SHOP_FIX) )
		result += "F";
	if ( BV_IS_SET(shop->shop_type, SHOP_RECHARGE) )
		result += "R";

	return result.empty() ? "Non" : result;
}


void do_shops( CHAR_DATA *ch, char *argument )
{
    SHOP_DATA *shop;

    if ( !first_shop )
    {
	send_to_char( "There are no shops.\n", ch );
	return;
    }

    set_char_color( AT_NOTE, ch );
    for ( shop = first_shop; shop; shop = shop->next )
	ch_printf( ch, "Keeper: %5d Buy: %3d Sell: %3d Fix: %3d Open: %2d Close: %2d Shops: %3s Buy: %s\n",
		shop->keeper,	   shop->profit_buy, shop->profit_sell, shop->profit_fix,
		shop->open_hour,   shop->close_hour,
		shop_type_to_string( shop ).c_str(),
		bitset_to_string(shop->buy_type, o_types).c_str() );
    return;
}


/* -------------- Repair Shop Building and Editing Section -------------- */


void do_makerepair( CHAR_DATA *ch, char *argument )
{
    REPAIR_DATA *repair;
    int vnum;
    MOB_INDEX_DATA *mob;

	send_to_char( "Repair shops have been combined with shops. Use makeshop to make a repair shops.\n", ch );
	return;

    if ( !argument || argument[0] == '\0' )
    {
	send_to_char( "Usage: makerepair <mobvnum>\n", ch );
	return;
    }

    vnum = atoi( argument );
    
    if ( (mob = get_mob_index(vnum)) == NULL )
    {
	send_to_char( "Mobile not found.\n", ch );
	return;
    }

    if ( !can_medit(ch, mob) )
      return;

    if ( mob->rShop )
    {
	send_to_char( "This mobile already has a repair shop.\n", ch );
	return;
    }

    CREATE( repair, REPAIR_DATA, 1 );
	repair->game = ch->game;

    LINK( repair, first_repair, last_repair, next, prev );
    repair->keeper	= vnum;
    repair->profit_fix	= 100;
    repair->shop_type	= SHOP_FIX;
    repair->open_hour	= 0;
    repair->close_hour	= 23;
    mob->rShop		= repair;
    send_to_char( "Done.\n", ch );
    return;
}


void do_repairset( CHAR_DATA *ch, char *argument )
{
    REPAIR_DATA *repair;
    MOB_INDEX_DATA *mob, *mob2;
    std::string arg1;
    std::string arg2;
	std::string argstr = argument;
    int vnum;
    int value;
    
	send_to_char( "Repair shops have been combined with shops. Use shopset to edit repair shops.\n", ch );
	return;

    argstr = one_argument( argstr, arg1 );
    argstr = one_argument( argstr, arg2 );

    if ( arg1.empty() || arg2.empty() )
    {
	send_to_char( "Usage: repairset <mob vnum> <field> value\n", ch );
	send_to_char( "\nField being one of:\n", ch );
	send_to_char( "  fixtype profit type open close keeper\n", ch );
	return;
    }

    vnum = strtoi( arg1 );

    if ( (mob = get_mob_index(vnum)) == NULL )
    {
	send_to_char( "Mobile not found.\n", ch );
	return;
    }

    if ( !can_medit(ch, mob) )
      return;

    if ( !mob->rShop )
    {
	send_to_char( "This mobile doesn't keep a repair shop.\n", ch );
	return;
    }
    repair = mob->rShop;
    value = strtoi( argstr );

    if ( !str_cmp( arg2, "fixtype" ) )
    {
	if ( !is_number(argstr) )
	  value = get_otype(argstr);
	if ( value < 0 || value > ITEMTYPE_MAX )
	{
	    send_to_char( "Invalid item type!\n", ch );
	    return;
	}
	BV_TOGGLE_BIT(repair->fix_type, value);
	send_to_char( "Done.\n", ch );
	return;
    }

    if ( !str_cmp( arg2, "profit" ) )
    {
	if ( value < 1 || value > 1000 )
	{
	    send_to_char( "Out of range.\n", ch );
	    return;
	}
	repair->profit_fix = value;
	send_to_char( "Done.\n", ch );
	return;
    }

    if ( !str_cmp( arg2, "type" ) )
    {
	if ( value < 1 || value > 2 )
	{
	    send_to_char( "Out of range.\n", ch );
	    return;
	}
	repair->shop_type = value;
	send_to_char( "Done.\n", ch );
	return;
    }

    if ( !str_cmp( arg2, "open" ) )
    {
	if ( value < 0 || value > 23 )
	{
	    send_to_char( "Out of range.\n", ch );
	    return;
	}
	repair->open_hour = value;
	send_to_char( "Done.\n", ch );
	return;
    }

    if ( !str_cmp( arg2, "close" ) )
    {
	if ( value < 0 || value > 23 )
	{
	    send_to_char( "Out of range.\n", ch );
	    return;
	}
	repair->close_hour = value;
	send_to_char( "Done.\n", ch );
	return;
    }

    if ( !str_cmp( arg2, "keeper" ) )
    {
	if ( (mob2 = get_mob_index(vnum)) == NULL )
	{
	  send_to_char( "Mobile not found.\n", ch );
	  return;
	}
	if ( !can_medit(ch, mob) )
	  return;
	if ( mob2->rShop )
	{
	  send_to_char( "That mobile already has a repair shop.\n", ch );
	  return;
	}
	mob->rShop  = NULL;
	mob2->rShop = repair;
	repair->keeper = value;
	send_to_char( "Done.\n", ch );
	return;
    }

    do_repairset( ch, "" );
    return;
}


void do_repairstat( CHAR_DATA *ch, char *argument )
{
    REPAIR_DATA *repair;
    MOB_INDEX_DATA *mob;
    int vnum;
    
	send_to_char( "Repair shops have been combined with shops. Use shopstat to show a repair shop.\n", ch );
	return;

    if ( argument[0] == '\0' )
    {
	send_to_char( "Usage: repairstat <keeper vnum>\n", ch );
	return;
    }

    vnum = atoi( argument );
    
    if ( (mob = get_mob_index(vnum)) == NULL )
    {
	send_to_char( "Mobile not found.\n", ch );
	return;
    }

    if ( !mob->rShop )
    {
	send_to_char( "This mobile doesn't keep a repair shop.\n", ch );
	return;
    }
    repair = mob->rShop;

    ch_printf( ch, "Keeper: %d  %s\n", repair->keeper, mob->short_descr );
    ch_printf( ch, "fixtype: %s\n",
			bitset_to_string(repair->fix_type, o_types).c_str() );
    ch_printf( ch, "Profit: %3d%%  Type: %d\n",
			repair->profit_fix,
			repair->shop_type );
    ch_printf( ch, "Hours:   open %2d  close %2d\n",
			repair->open_hour,
			repair->close_hour );
    return;
}


void do_repairshops( CHAR_DATA *ch, char *argument )
{
    REPAIR_DATA *repair;

	send_to_char( "Repair shops have been combined with shops. Use shops to show repair shops.\n", ch );
	return;

    if ( !first_repair )
    {
	send_to_char( "There are no repair shops.\n", ch );
	return;
    }

    set_char_color( AT_NOTE, ch );
    for ( repair = first_repair; repair; repair = repair->next )
	ch_printf( ch, "Keeper: %5d Profit: %3d Type: %d Open: %2d Close: %2d Fix: %s\n",
		repair->keeper,	     repair->profit_fix, repair->shop_type,
		repair->open_hour,   repair->close_hour,
		bitset_to_string(repair->fix_type, o_types).c_str() );
    return;
}

/*This is the command used to buy a contract from a vendor to place a player
owned vendor
*/
void do_buyvendor (CHAR_DATA *ch, char *argument)
{
  CHAR_DATA *keeper;
  OBJ_DATA *deed;
  std::string buf, buf1, strsave;
  struct stat fst;

  if (IS_NPC(ch))
    return;

  if ( !str_cmp( argument, "yes" ) )
  {
    buf = str_printf("%s/%s", VENDOR_DIR, capitalize( ch->name ).c_str() );
    remove( buf.c_str() );
  }
  
    strsave = str_printf("%s/%s", VENDOR_DIR, capitalize( ch->name ).c_str() );

    	if ( stat( strsave.c_str(), &fst ) != -1 )
    	{
		send_to_char( "You already have a shop!\n", ch);
		send_to_char( "If you want to buy one anyway, type buyvendor yes.\n", ch);
		send_to_char( "Your old one will be deleted.\n", ch);
		return;
	}
  

  if (  (keeper = find_keeper_q( ch, FALSE ) )  == NULL  )
  {
    send_to_char ("There is no one to buy that from!\n", ch);
    return;
  }

  if ( ch->gold < COST_BUY_VENDOR )
     {
         buf1 = str_printf("%s says, You are too poor!\n", keeper->name);
         send_to_char (buf1, ch);
         return;
     }

if ( (ch->top_level) < LEVEL_BUY_VENDOR )
	{
		buf1 = str_printf("you must be at least %d level.\n", LEVEL_BUY_VENDOR);
		send_to_char (buf1, ch);
		return;
	}

if ( (get_obj_index (OBJ_VNUM_DEED) ) == NULL )
	{
		bug ("BUYVENDOR: Deed is missing!");
		return;
	}

deed = create_object ( get_obj_index(OBJ_VNUM_DEED), 0);
obj_to_char (deed, ch);
send_to_char ("&bVery well, you may have a contract for a vendor.\n", ch);
ch->gold = ch->gold - COST_BUY_VENDOR;

}


/*this is the command that places the vendor at the specified location*/
void do_placevendor (CHAR_DATA *ch, char *argument)
{
std::string strsave;
struct stat fst;
CHAR_DATA *vendor;
MOB_INDEX_DATA *temp;
OBJ_DATA *obj;
std::string vnum1;
char buf[MAX_STRING_LENGTH]; // Need this one to handle obj desc token format

if (  find_keeper_q (ch, FALSE) )
{
send_to_char ("A vendor is already here!\n",ch);
return;
}

if ( IS_NPC(ch) )
	return;



/* better way to do this? what if they have another object called deed?*/
  if ( ( obj = get_obj_carry( ch, "deed" ) ) == NULL )
	{
	    send_to_char( "You do not have a deed!.\n", ch );
	    return;
	}

  if (obj->pIndexData->vnum != OBJ_VNUM_DEED)
	{
	    send_to_char( "You do not have a deed!.\n", ch );
	    return;
	}

	if (!BV_IS_SET(ch->in_room->room_flags, ROOM_PLR_SHOP) )
	{
		send_to_char( "You need to find a free shop.\n", ch);
		return;
	}

        strsave = str_printf( "%s/%s", VENDOR_DIR, capitalize( ch->name ).c_str() );

    	if ( stat( strsave.c_str(), &fst ) != -1 )
    	{
		send_to_char( "You already have a shop!\n", ch);
		return;
	}



if ( (temp = get_mob_index (MOB_VNUM_VENDOR) ) == NULL )
{
	log_string ("do_placevendor: no vendor vnum");
	return;
}

if (!ch->game)
{
    send_to_char( "You must be in a game to place a vendor.\n", ch );
    return;
}

char_to_room( create_mobile( ch->game, temp ), ch->in_room );
vendor = get_char_room(ch, temp->player_name);

SPRINTF_RUNTIME (buf, vendor->long_descr, ch->name);
vendor->long_descr =  STRALLOC( buf );

SPRINTF (buf, "%s", ch->name);

vendor->owner = STRALLOC(buf);
vendor->home = ch->in_room;

save_vendor (vendor);

separate_obj( obj );
extract_obj( obj );

act( AT_ACTION, "$n appears in a swirl of smoke.\n", vendor, NULL, NULL, TO_ROOM );

vnum1 = str_printf("%d", vendor->pIndexData->vnum);
do_makeshop (vendor, (char*)vnum1.c_str() ); /*makes the vendor a shop.. there has to be a
better way to do it but hell if i know what it is!*/

}



void do_pricevendor (CHAR_DATA *ch, char *argument)
{
CHAR_DATA *vendor;
CHAR_DATA *ch1;
std::string arg1;
std::string arg2;
std::string argstr = argument;
OBJ_DATA *obj;
struct tm *tms;

argstr = one_argument (argstr, arg1);
argstr = one_argument (argstr, arg2);

if ( arg1.empty() )
	{
	    send_to_char("usage:> pricevendor <item> <cost>\n",ch);
	    return;
	}

if (arg2.empty())
	{
		send_to_char("usage:> pricevendor <item> <cost>\n",ch);
		return;
	}


if ( ( vendor = find_keeper (ch) ) == NULL )
{
return;
}

if ( !(vendor->owner) )
  return;

if ( (ch1 = get_char_room(ch, vendor->owner)) == NULL )
{
send_to_char ("This isnt your vendor!\n",ch);
return;
}
if ( str_cmp( ch1->name, vendor->owner ) )
	{
          send_to_char ("Trying to steal huh?\n",ch);
 	  tms = localtime(&current_time);
 	  tms->tm_hour += 24;
	  ch->pcdata->release_date = mktime(tms);
	  ch->pcdata->helled_by = STRALLOC("VendorCheat");
	  act(AT_MAGIC, "$n disappears in a cloud of hellish light.", ch, NULL, ch, TO_NOTVICT);
	  char_from_room(ch);
	  char_to_room(ch, get_room_index(6));
	  act(AT_MAGIC, "$n appears in a could of hellish light.", ch, NULL, ch, TO_NOTVICT);
	  do_look(ch, "auto");
	  ch_printf(ch, "The immortals are not pleased with your actions.\n"
            "You shall remain in hell for 24 Hours.\n");
	  save_char_obj(ch);	/* used to save ch, fixed by Thoric 09/17/96 */
          log_printf( "%s just tried to abuse the vendor bug!" , ch->name);
	  return;
	}

if ( ch->fighting)
{
send_to_char ("Not while you fightZ!\n",ch);
return;
}

if ( (obj  = get_obj_carry( vendor, arg1 )) != NULL)
	{
		obj->cost = strtoi (arg2);
		send_to_char("The price has been changed\n",ch);
		save_vendor(vendor);
		return;
	}


send_to_char("He doesnt have that item!\n",ch);
save_vendor(vendor);
return;

}


void do_collectgold (CHAR_DATA *ch, char *argument)
{
CHAR_DATA *vendor;
CHAR_DATA *ch1;
long gold;
std::string name;
  struct tm *tms;

if ( ( vendor = find_keeper (ch) ) == NULL )
{
return;
}

if (vendor->owner == NULL)
	{
		send_to_char("thats not a vendor!\n",ch);
		return;
    }

name = vendor->owner;

if ( (ch1 = get_char_room(ch, vendor->owner)) == NULL )
	{
		send_to_char ("Trying to steal huh?\n",ch);
		return;
	}
if ( str_cmp( ch1->name, vendor->owner ) )
	{
          send_to_char ("Trying to steal huh?\n",ch);
 	  tms = localtime(&current_time);
 	  tms->tm_hour += 24;
	  ch->pcdata->release_date = mktime(tms);
	  ch->pcdata->helled_by = STRALLOC("VendorCheat");
	  act(AT_MAGIC, "$n disappears in a cloud of hellish light.", ch, NULL, ch, TO_NOTVICT);
	  char_from_room(ch);
	  char_to_room(ch, get_room_index(6));
	  act(AT_MAGIC, "$n appears in a could of hellish light.", ch, NULL, ch, TO_NOTVICT);
	  do_look(ch, "auto");
	  ch_printf(ch, "The immortals are not pleased with your actions.\n"
            "You shall remain in hell for 24 Hours.\n");
	  save_char_obj(ch);	/* used to save ch, fixed by Thoric 09/17/96 */
          log_printf( "%s just tried to abuse the vendor bug!" , ch->name);
	  return;
	}


if ( !(ch == ch1) )
{
	log_printf( "collectgold: %s and ch1 = %s\n", name.c_str(), ch1->name);
	send_to_char ("This isnt your vendor!\n",ch);
	return;
}

if ( ch->fighting)
{
send_to_char ("Not while you fightZ!\n",ch);
return;
}

gold = vendor->gold;
gold -= (int) (gold * VENDOR_FEE);
if( vendor->in_room && vendor->in_room->area )
  boost_economy( vendor->in_room->area, vendor->gold);
vendor->gold = 0;
ch->gold += gold;

send_to_char ("&GYour vendor gladly hands over his earnings minus a small fee of course..\n",ch);
act( AT_ACTION, "$n hands over some money.\n", vendor, NULL, NULL, TO_ROOM );

save_vendor(vendor);
}


/* Write vendor to file */
void fwrite_vendor( FILE *fp, CHAR_DATA *mob )
{
  if ( !IS_NPC( mob ) || !fp )
	return;
  fprintf( fp, "Vnum     %d\n", mob->pIndexData->vnum );
  if (mob->gold > 0)
  fprintf (fp, "Gold     %d\n",mob->gold);
  if ( mob->home )
  fprintf( fp, "Home     %d\n", mob->home->vnum );
  if (mob->owner != NULL )
  fprintf (fp, "Owner     %s~\n", mob->owner );
  if ( str_cmp( mob->short_descr, mob->pIndexData->short_descr) )
  	fprintf( fp, "Short	    %s~\n", mob->short_descr );
  fprintf( fp, "Position   %d\n", mob->position );
  fwrite_bitset(fp, "FlagsEx",mob->act);
//  fprintf( fp, "Flags   %d\n",   mob->act );
fprintf( fp, "END\n" );

  return;
}


/* read vendor from file */
CHAR_DATA *  fread_vendor( GameContext *game, FILE *fp )
{
  CHAR_DATA *mob = NULL;

  const char *word;
  bool fMatch;
  int inroom = 0;
  ROOM_INDEX_DATA *pRoomIndex = NULL;
  CHAR_DATA *victim;
  CHAR_DATA *vnext;
char buf [MAX_INPUT_LENGTH];
char vnum1 [MAX_INPUT_LENGTH];
  word   = feof( fp ) ? "END" : fread_word( fp );
  if ( !strcmp(word, "Vnum") )
  {
    int vnum;

    vnum = fread_number( fp );
    mob = create_mobile( game, get_mob_index(vnum));
    if ( !mob )
    {
	for ( ; ; )
		{
	  word   = feof( fp ) ? "END" : fread_word( fp );
	  if ( !strcmp( word, "END" ) )
		break;
        }
	bug("Fread_mobile: No index data for vnum %d", vnum );
	return NULL;
    }
  }
  else
  {
	for ( ; ; )
		{
	  word   = feof( fp ) ? "END" : fread_word( fp );
	  if ( !strcmp( word, "END" ) )
		break;
        }
	extract_char(mob, TRUE);
	bug("Fread_vendor: Vnum not found", 0 );
	return NULL;
  }
  for ( ; ;) {
       word   = feof( fp ) ? "END" : fread_word( fp );
       fMatch = FALSE;
       switch ( UPPER(word[0]) ) {
	case '*':
           fMatch = TRUE;
           fread_to_eol( fp );
           break;
	case '#':
		if ( !strcmp( word, "#OBJECT" ) )
			{
			fread_obj ( game, mob, fp, OS_CARRY );
			}
			break;
	case 'D':
		KEY( "Description", mob->description, fread_string(fp));
		break;
	case 'E':

		if ( !strcmp( word, "END" ) )
		{
		if ( inroom == 0 )
			inroom = ROOM_VNUM_VENSTOR;
		mob->home = get_room_index(inroom);
		pRoomIndex = get_room_index( inroom );
		if ( !pRoomIndex )
			{
			pRoomIndex = get_room_index( ROOM_VNUM_VENSTOR );
			mob->home = get_room_index( ROOM_VNUM_VENSTOR );
		    }

	  mob->in_room = pRoomIndex;
	  /* the following code is to make sure no more then one player owned vendor
	  is in the room - meckteck */
	  for ( victim = mob->in_room->first_person; victim; victim = vnext )
			{
		vnext = victim->next_in_room;
		if (victim->home != NULL)
		{
		extract_char( victim, TRUE);
        break;
		}

           }

		char_to_room(mob, pRoomIndex);
		SPRINTF(vnum1,"%d", mob->pIndexData->vnum);
         do_makeshop (mob, vnum1 );
		SPRINTF_RUNTIME (buf, mob->long_descr, mob->owner);
         mob->long_descr =  STRALLOC( buf );
		 mob->hit = 10000;
		 mob->max_hit = 10000;
		return mob;
		}
		break;
 	case 'F':
		//KEY( "Flags", mob->act, fread_number(fp));
		if ( !str_cmp( word, "Flags"  ) )
		{
			mob->act = int_to_bitset(fread_number( fp ));
			fMatch = TRUE;
			break;
		}
		if ( !str_cmp( word, "FlagsEx"  ) )
		{
			fread_bitset(fp, mob->act);
			fMatch = TRUE;
			break;
		}		
		break;
	case 'G':
	KEY("Gold", mob->gold, fread_number(fp));
	break;
   case 'H':
	  KEY("Home", inroom, fread_number(fp) );
     break;
	case 'L':
		break;
	case 'N':
		break;
	case 'O':
		KEY ("Owner", mob->owner, fread_string (fp) );
		break;
	case 'P':
		KEY( "Position", mob->position, fread_number( fp ) );
		break;
	case 'S':
		KEY( "Short", mob->short_descr, fread_string(fp));
		break;
	}
	if ( !fMatch )
	{
	   bug ( "Fread_mobile: no match.", 0 );
	   bug ( word, 0 );
	}
  }
  return NULL;
}




void save_vendor( CHAR_DATA *ch )
{
    std::string strsave;
    FILE *fp;

    if ( !ch )
    {
	bug( "Save_char_obj: null ch!", 0 );
	return;
    }

    de_equip_char( ch );


    strsave = str_printf("%s%s",VENDOR_DIR, capitalize( ch->owner ).c_str() );

    if ( ( fp = fopen( strsave.c_str(), "w" ) ) == NULL )
    {
	perror( strsave.c_str() );
	bug( "Save_vendor: fopen", 0 );
    }
    else
    {
	bool ferr;

	chmod(strsave.c_str(), S_IRUSR|S_IWUSR | S_IRGRP|S_IWGRP | S_IROTH|S_IWOTH);
	fprintf( fp, "#VENDOR\n"		);
	fwrite_vendor( fp, ch );

	if ( ch->first_carrying )
	fwrite_obj( ch, ch->last_carrying, fp, 0, OS_CARRY );

	fprintf(fp, "#END\n" );
	ferr = ferror(fp);
	FCLOSE( fp );
	if (ferr)
	{
	  perror(strsave.c_str());
	  bug("Error writing temp file for %s -- not copying", strsave.c_str());
	}
    }

    re_equip_char( ch );
    return;
}

