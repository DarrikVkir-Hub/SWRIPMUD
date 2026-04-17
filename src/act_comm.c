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
*			   Player communication module     		   *
****************************************************************************/


#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "mud.h"

/*
 *  Externals
 */
void send_obj_page_to_char(CHAR_DATA * ch, OBJ_INDEX_DATA * idx, char page);
void send_room_page_to_char(CHAR_DATA * ch, ROOM_INDEX_DATA * idx, char page);
void send_page_to_char(CHAR_DATA * ch, MOB_INDEX_DATA * idx, char page);
void send_control_page_to_char(CHAR_DATA * ch, char page);

/*
 * Local functions.
 */
void	talk_channel	args( ( CHAR_DATA *ch, const std::string& argument,
			    int channel, const std::string& verb ) );

std::string scramble(const std::string& argument, int modifier);
std::string drunk_speech(const std::string& argument, CHAR_DATA *ch);

int lang_sn[LANG_MAX];

typedef struct channel_info
{
    int channel;

    /* behavior flags (LEFT SIDE: fast scanning) */
    bool requires_comlink;
    bool imm_only;
    int  command_group;

    bool clan_only, allclan, order_only, guild_only;
    bool room_only, ship_channel;

    bool no_scramble;
    bool use_drunk;

    bool uses_verb;
    bool use_act_to_char;

    const char *name;
    int color;

    const char *prefix_fmt;  //optional prefix (can include $n, etc.)
    const char *self_fmt;    // ch_printf 
    const char *other_fmt;   // act()/snprintf

} CHANNEL_INFO;

#ifndef T
#define T true
#define F false
#endif

static const CHANNEL_INFO channel_table[] =
{
//  channel             com imm                           allclan       guild      ship      drunk     act                  color 
//  channel             com     cgroup               clan         order       room      no_scr    verb       name            
    { CHANNEL_CHAT,     T,  F,  CGROUP_NONE,         F,   F,      F,    F,    F,   F,   F,   F,   T,   F,   "chat",         AT_GOSSIP,  
      NULL,                                                                         // <---- prefix_fmt                 - Prefix sent before message to others
      "&z&CYou %s over the public network in %s&c, '&C%s&c'\n",                     // <---- self_fmt (ch_printf)       - message to self
      "&z&C$n &C%ss over the public network in $l&c, '&C$t&c'" },                   // <---- other_fmt (act()/snprintf) - message to others

    { CHANNEL_OOC,      F,  F,  CGROUP_NONE,         F,   F,      F,    F,    F,   F,   T,   F,   F,   T,   "ooc",          AT_OOC,
      "&z&g(&GOOC&g)&Y",
      NULL,
      "$n&Y: $t" },

    { CHANNEL_IMMTALK,  F,  T,  CGROUP_NONE,         F,   F,      F,    F,    F,   F,   T,   F,   F,   T,   "immtalk",      AT_IMMORT,
      NULL,
      NULL,
      "$n&Y>&W $t" },

    { CHANNEL_AVTALK,   F,  F,  CGROUP_NONE,         F,   F,      F,    F,    F,   F,   T,   T,   F,   T,   "avtalk",       AT_AVATAR,
      "&z&B<&cAvatar&B>&c",
      NULL,
      "$n&c: $t" },

    { CHANNEL_CLAN,     T,  F,  CGROUP_NONE,         T,   F,      F,    F,    F,   F,   F,   F,   F,   F,   "clan",         AT_CLAN,
      NULL,
      "&z&POver the organizations private network you say in %s&R, '&P%s&R'\n",
      "&z&P$n &Pspeaks over the organizations network in $l&R, '&P$t&R'" },

    { CHANNEL_ALLCLAN,  T,  F,  CGROUP_NONE,         T,   T,      F,    F,    F,   F,   F,   F,   F,   F,   "allclan",      AT_CLAN,
      NULL,
      "&z&POver the entire organizations private network you say in %s&R, '&P%s&R'\n",
      "&z&P$n &Pspeaks over the entire organizations network in $l&R, '&P$t&R'" },

    { CHANNEL_ORDER,    T,  F,  CGROUP_NONE,         F,   F,      T,    F,    F,   F,   F,   F,   F,   F,   "order",        AT_GOSSIP,
      NULL,
      "&z&CYou %s over the order network in %s&c, '&C%s&c'\n",
      "&z&C$n &C%ss over the order network in $l&c, '&C$t&c'" },

    { CHANNEL_GUILD,    T,  F,  CGROUP_NONE,         F,   F,      F,    T,    F,   F,   F,   F,   F,   F,   "guild",        AT_GOSSIP,
      NULL,
      "&z&CYou %s over the guild network in %s&c, '&C%s&c'\n",
      "&z&C$n &C%ss over the guild network in $l&c, '&C$t&c'" },

    { CHANNEL_SHOUT,    F,  F,  CGROUP_NONE,         F,   F,      F,    F,    T,   F,   F,   T,   T,   F,   "shout",        AT_GOSSIP,
      NULL,
      "You %s in %s, '%s'\n",
      "$n %ss in $l, '$t'" },

    { CHANNEL_YELL,     F,  F,  CGROUP_NONE,         F,   F,      F,    F,    T,   F,   F,   T,   T,   F,   "yell",         AT_GOSSIP,
      NULL,
      "You %s in %s, '%s'\n",
      "$n %ss in $l, '$t'" },

    { CHANNEL_ASK,      F,  F,  CGROUP_NONE,         F,   F,      F,    F,    F,   F,   T,   F,   T,   F,   "ask",          AT_OOC,
      "&z&g(&GOOC&g)&Y",
      "&z&g(&GOOC&g)&Y You %s, '%s'\n",
      "$n &Y%ss, '$t'" },

    { CHANNEL_NEWBIE,   F,  F,  CGROUP_NONE,         F,   F,      F,    F,    F,   F,   T,   F,   F,   T,   "newbie",       AT_OOC,
      "&z&r(&RNEWBIE&r)&Y",
      NULL,
      "$n&Y: $t" },

    { CHANNEL_NEWBIEASST,F, F,  CGROUP_NONE,         F,   F,      F,    F,    F,   F,   T,   F,   F,   F,   "newbieasst",   AT_OOC,
      "&z&B(&G*&BNewbie Helper&G*&B)&G",
      "&z&B(&G*&BNewbie Helper&G*&B)&G %s&Y You say: %s\n",
      "$n&Y: $t" },

    { CHANNEL_VULGAR,   T,  F,  CGROUP_NONE,         F,   F,      F,    F,    F,   F,   F,   F,   F,   T,   "vulgar",       AT_VULGAR,
      "&z&R<&WVULGAR&R> &W",
      NULL,
      "$n&R: &W'&R$t&W'" },

    { CHANNEL_SHIP,     F,  F,  CGROUP_NONE,         F,   F,      F,    F,    F,   T,   F,   F,   F,   F,   "ship",         AT_SHIP,
      NULL,
      "&z&rYou tell the ship in %s&P, '%s'\n",
      "&z&r$n &rsays over the ships com system in $l,&P '$t'" },

    { CHANNEL_SYSTEM,   F,  F,  CGROUP_NONE,         F,   F,      F,    F,    F,   T,   F,   F,   F,   F,   "system",       AT_GOSSIP,
      "&z&R(System)",
      "&z&R(System) (%s): '&W%s&r'\n",
      "$n&r: '&W$t&r'" },

    { CHANNEL_SPACE,    F,  F,  CGROUP_NONE,         F,   F,      F,    F,    F,   T,   F,   F,   F,   F,   "space",        AT_GOSSIP,
      "&z&g(&GOOC&g)&R(Space)",
      "&z&rYou space &g(&GOOC&g):, '&W%s&r'\n",
      "$n&r: '&W$t&r'" },

    { CHANNEL_103,      F,  F,  CGROUP_ADMIN,        F,   F,      F,    F,    F,   F,   T,   F,   F,   T,   "admin",        AT_IMMORT,
      "&z&Y(&WAdmin&Y)&W",
      NULL,
      "$n&Y>&W $t" },

    { CHANNEL_104,      F,  F,  CGROUP_HEAD_ADMIN,   F,   F,      F,    F,    F,   F,   T,   F,   F,   T,   "headadmin",    AT_IMMORT,
      "&z&Y(&WHeadAdmin&Y)&W",
      NULL,
      "$n&Y>&W $t" },

    { CHANNEL_105,      F,  F,  CGROUP_IMPLEMENTOR,  F,   F,      F,    F,    F,   F,   T,   F,   F,   T,   "imp",          AT_IMMORT,
      "&z&Y(&WImp&Y)&W",
      NULL,
      "$n&Y>&W $t" },

    { CHANNEL_NEWS,     F,  F,  CGROUP_NONE,         F,   F,      F,    F,    F,   F,   T,   F,   F,   F,   "news",         AT_GOSSIP,
      "&W[&RICNN&W]",
      NULL,
      "%s\n" },
};

static const CHANNEL_INFO *get_channel_info(int channel)
{
    for (size_t i = 0; i < sizeof(channel_table)/sizeof(channel_table[0]); i++)
        if (channel_table[i].channel == channel)
            return &channel_table[i];

    return NULL;
}

const FLAG_SET &valid_langs()
{
    static FLAG_SET bv;
    static bool initialized = false;

    if (!initialized)
    {
        initialized = true;

        bv.set(LANG_COMMON);
        bv.set(LANG_WOOKIEE);
        bv.set(LANG_TWI_LEK);
        bv.set(LANG_RODIAN);
        bv.set(LANG_HUTT);
        bv.set(LANG_MON_CALAMARI);
        bv.set(LANG_NOGHRI);
        bv.set(LANG_GAMORREAN);
        bv.set(LANG_JAWA);
        bv.set(LANG_ADARIAN);
        bv.set(LANG_EWOK);
        bv.set(LANG_VERPINE);
        bv.set(LANG_DEFEL);
        bv.set(LANG_TRANDOSHAN);
        bv.set(LANG_CHADRA_FAN);
        bv.set(LANG_QUARREN);
        bv.set(LANG_SULLUSTAN);
        bv.set(LANG_BARABEL);
        bv.set(LANG_FIRRERREO);
        bv.set(LANG_BOTHAN);
        bv.set(LANG_TOGORIAN);
        bv.set(LANG_KUBAZ);
        bv.set(LANG_YEVETHAN);
        bv.set(LANG_GAND);
        bv.set(LANG_DUROS);
        bv.set(LANG_COYNITE);
        bv.set(LANG_GOTAL);
        bv.set(LANG_DEVARONIAN);
        bv.set(LANG_FALLEEN);
        bv.set(LANG_ITHORIAN);
        bv.set(LANG_BINARY);
        bv.set(LANG_DIVINE);
    }

    return bv;
}

bool is_valid_lang(size_t lang)
{
    return valid_langs().test(lang);
}

FLAG_SET make_all_languages(void)
{
    FLAG_SET bs;
    int lang;

    bs.reset();
    for (lang = 0; lang < LANG_MAX; ++lang)
        BV_SET_BIT(bs, lang);

    return bs;
}


void sound_to_room( ROOM_INDEX_DATA *room , const std::string& argument )
{
   CHAR_DATA *vic;

        if ( room == NULL ) return;
        
        for ( vic = room->first_person; vic; vic = vic->next_in_room )
	   if ( !IS_NPC(vic) && BV_IS_SET( vic->act, PLR_SOUND ) )
	     send_to_char( argument, vic );
     
}

std::string lang_string(CHAR_DATA *ch,  CHAR_DATA *vch)
{
    if (!ch || !vch)
        return "??";

    int lang = ch->speaking;

    if (!IS_NPC(vch) &&
        knows_language(vch, lang, ch) &&
        (!IS_NPC(ch) || lang != LANG_COMMON))
    {
        if (lang >= 0 && lang < LANG_MAX )
        {
            std::string result = get_flag_name(lang_names, lang, LANG_MAX);
            if (!result.empty())
                result[0] = toupper(result[0]);
            return result;
        }
    }

    return "??";
}

void do_beep( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    std::string arg;
    std::string argumentstr;
    OBJ_DATA *obj;
    bool ch_comlink, victim_comlink;
    
    argumentstr = one_argument( argument, arg );
    
    BV_SET_BIT( ch->channels, CHANNEL_TELLS );
    if (!ch || !ch->in_room) return;
    if ( BV_IS_SET( ch->in_room->room_flags, ROOM_SILENCE ) )
    {
        send_to_char( "You can't do that here.\n", ch );
        return;
    }
                                
    if (!IS_NPC(ch)
        && ( BV_IS_SET(ch->act, PLR_SILENCE)
        ||   BV_IS_SET(ch->act, PLR_NO_TELL) ) )
    {
         send_to_char( "You can't do that.\n", ch );
         return;
    }
                                    
    if ( arg.empty() )
    {
         send_to_char( "Beep who?\n", ch );
         return;
    }
                            
    if ( ( victim = get_char_world( ch, arg ) ) == NULL 
       || ( IS_NPC(victim) && victim->in_room != ch->in_room ) 
       || (!NOT_AUTHED(ch) && NOT_AUTHED(victim) && !IS_IMMORTAL(ch) ) )
    {
	send_to_char( "They aren't here.\n", ch );
	return;
    }
    
      ch_comlink = FALSE;
      victim_comlink = FALSE;
      
      if ( IS_IMMORTAL( ch ) )
      {
         ch_comlink = TRUE;
         victim_comlink = TRUE;
      }
      
      if ( IS_IMMORTAL( victim ) )
         victim_comlink = TRUE;
      
      for ( obj = ch->last_carrying; obj; obj = obj->prev_content )
      {
        if (obj->pIndexData->item_type == ITEM_COMLINK)
        ch_comlink = TRUE;
      }
    
      if ( !ch_comlink )
      {
	send_to_char( "You need a comlink to do that!\n", ch);
	return;
      }
      
      for ( obj = victim->last_carrying; obj; obj = obj->prev_content )
      {
        if (obj->pIndexData->item_type == ITEM_COMLINK)
        victim_comlink = TRUE;
      }

      if ( !victim_comlink )
      {
	send_to_char( "They don't seem to have a comlink!\n", ch);
	return;
      }
    
    if (NOT_AUTHED(ch) && !NOT_AUTHED(victim) && !IS_IMMORTAL(victim) )
    {
	send_to_char( "They can't hear you because you are not authorized.\n", ch);
	return;
    }

    if ( !IS_NPC( victim ) && ( victim->switched ) 
	&& ( get_trust( ch ) > LEVEL_AVATAR ) )
    {
      send_to_char( "That player is switched.\n", ch );
      return;
    }

   else if ( !IS_NPC( victim ) && ( !victim->desc ) )
    {
      send_to_char( "That player is link-dead.\n", ch );
      return;
    }

    if ( !BV_IS_SET( victim->channels, CHANNEL_TELLS ) 
    && ( !IS_IMMORTAL( ch ) || ( get_trust( ch ) < get_trust( victim ) ) ) )
    {
      act( AT_PLAIN, "$E has $S tells turned off.", ch, NULL, victim,
		TO_CHAR );
      return;
    }

    if ( !IS_NPC (victim) && ( BV_IS_SET (victim->act, PLR_SILENCE ) ) )
      {
      send_to_char( "That player is silenced.  They will receive your message but can not respond.\n", ch );
      }   

    if (!ch || !ch->in_room) return;
    if ( (!IS_IMMORTAL(ch) && !IS_AWAKE(victim) )
    || (!IS_NPC(victim) && BV_IS_SET(victim->in_room->room_flags, ROOM_SILENCE ) ) )
    {
    act( AT_PLAIN, "$E can't hear you.", ch, 0, victim, TO_CHAR );
	return;
    }

    if ( victim->desc		/* make sure desc exists first  -Thoric */
    &&   victim->desc->connected == CON_EDITING 
    &&   get_trust(ch) < LEVEL_GOD )
    {
	act( AT_PLAIN, "$E is currently in a writing buffer.  Please try again in a few minutes.", ch, 0, victim, TO_CHAR );
        return;
    }

    ch_printf(ch , "&WYou beep %s: %s\n\a" , victim->name, argumentstr.c_str() );
    send_to_char("\a",victim);    

    if ( knows_language( victim, ch->speaking, ch )
    ||  (IS_NPC(ch) && !ch->speaking) )
	act( AT_WHITE, std::string("$n beeps: '$t'"), ch, argumentstr, victim, TO_VICT );
    else
	act( AT_WHITE, "$n beeps: '$t'", ch, scramble(argumentstr, ch->speaking), victim, TO_VICT );
}

/* Text scrambler -- Altrag */
std::string scramble(const std::string& argument, int modifier)
{
    std::string result;
    result.reserve(argument.size());

    int conversion = 0;

    modifier %= number_range(80, 300);

    for (size_t i = 0; i < argument.size(); )
    {
        size_t len = utf8_char_len_safe(&argument[i]);

        /* Preserve UTF-8 multibyte characters */
        if (len > 1)
        {
            result.append(argument, i, len);
            i += len;
            continue;
        }

        char c = argument[i];

        if (c >= 'A' && c <= 'Z')
        {
            conversion = -conversion + static_cast<int>(i) - modifier + c - 'A';
            conversion = number_range(conversion - 5, conversion + 5);

            while (conversion > 25) conversion -= 26;
            while (conversion < 0)  conversion += 26;

            result.push_back(static_cast<char>(conversion + 'A'));
        }
        else if (c >= 'a' && c <= 'z')
        {
            conversion = -conversion + static_cast<int>(i) - modifier + c - 'a';
            conversion = number_range(conversion - 5, conversion + 5);

            while (conversion > 25) conversion -= 26;
            while (conversion < 0)  conversion += 26;

            result.push_back(static_cast<char>(conversion + 'a'));
        }
        else if (c >= '0' && c <= '9')
        {
            conversion = -conversion + static_cast<int>(i) - modifier + c - '0';
            conversion = number_range(conversion - 2, conversion + 2);

            while (conversion > 9) conversion -= 10;
            while (conversion < 0) conversion += 10;

            result.push_back(static_cast<char>(conversion + '0'));
        }
        else
        {
            result.push_back(c);
        }

        i += 1;  // ASCII advance
    }

    return result;
}

std::string drunk_speech(const std::string& argument, CHAR_DATA *ch)
{
    if (argument.empty())
        return "";

    if (!ch || IS_NPC(ch) || !ch->pcdata)
        return argument;

    int drunk = ch->pcdata->condition[COND_DRUNK];

    if (drunk <= 0)
        return argument;

    std::string result;
    result.reserve(argument.size() * 2); // rough guess for expansion

    const char *in = argument.c_str();

    while (*in)
    {
        size_t len = utf8_char_len_safe(in);

        /* Preserve UTF-8 multibyte chars untouched */
        if (len > 1)
        {
            result.append(in, len);
            in += len;
            continue;
        }

        /* ASCII path (original logic) */
        char c = *in++;

        /* slur S */
        if (toupper((unsigned char)c) == 'S' && number_percent() < drunk * 2)
        {
            result.push_back(c);
            result.push_back('h');
            continue;
        }

        /* slur X -> csh */
        if (toupper((unsigned char)c) == 'X' && number_percent() < drunk)
        {
            result.push_back('c');
            result.push_back('s');
            result.push_back('h');
            continue;
        }

        /* duplicate letters randomly */
        if (number_percent() < drunk / 2)
        {
            int repeat = number_range(1, 2);
            while (repeat--)
                result.push_back(c);
            continue;
        }

        /* random caps flip */
        if (number_percent() < drunk)
        {
            if (islower((unsigned char)c))
                c = toupper((unsigned char)c);
            else if (isupper((unsigned char)c))
                c = tolower((unsigned char)c);
        }

        result.push_back(c);

        /* stutter at spaces */
        if (c == ' ' && number_percent() < drunk / 2)
        {
            const char *peek = in;
            int count = number_range(1, 3);

            while (*peek && *peek != ' ' && count--)
            {
                size_t plen = utf8_char_len_safe(peek);

                result.append(peek, plen);
                result.push_back('-');

                peek += plen;
            }
        }
    }

    return result;
}

static bool has_comlink(CHAR_DATA *ch)
{
    if ( IS_IMMORTAL(ch) )
        return true;

    for ( OBJ_DATA *obj = ch->last_carrying; obj; obj = obj->prev_content )
        if ( obj->pIndexData->item_type == ITEM_COMLINK )
            return true;

    return false;
}

static bool can_receive_channel(
    CHAR_DATA *ch,
    CHAR_DATA *vch,
    CHAR_DATA *och,
    const CHANNEL_INFO *info,
    CLAN_DATA *clan)
{
    if (!vch || !vch->in_room)
        return false;

    if (BV_IS_SET(vch->in_room->room_flags, ROOM_SILENCE))
        return false;

    if (info->requires_comlink && !has_comlink(och))
        return false;

    if (info->imm_only && !IS_IMMORTAL(och))
        return false;

    if (info->command_group != CGROUP_NONE &&
        !BV_IS_SET(och->pcdata->commandgroup, info->command_group))
        return false;

    if (info->room_only && ch->in_room != och->in_room)
        return false;

    if (info->clan_only)
    {
        if (IS_NPC(vch) || !vch->pcdata || !vch->pcdata->clan)
            return false;

        if (!info->allclan && vch->pcdata->clan != clan)
            return false;

        if (info->allclan)
        {
            if (vch->pcdata->clan != clan
            && vch->pcdata->clan->mainclan != clan
            && clan->mainclan != vch->pcdata->clan)
                return false;
        }
    }

    if (info->ship_channel)
    {
        SHIP_DATA *ship = ship_from_cockpit(ch->game,ch->in_room->vnum);
        SHIP_DATA *target = ship_from_cockpit(ch->game, vch->in_room->vnum);

        if (!ship || !target)
            return false;

        if (info->channel == CHANNEL_SHIP)
            if (vch->in_room->vnum < ship->get_firstroom() || vch->in_room->vnum > ship->get_lastroom())
                return false;

        if (info->channel == CHANNEL_SYSTEM && !ship_in_range(ship, target))
            return false;
    }

    return true;
}

static void format_channel_message(
    CHAR_DATA *ch,
    int channel,
    const std::string &verb,
    const std::string &argument,
    std::string &buf )
{
    buf.clear();

    const CHANNEL_INFO *info = get_channel_info( channel );
    if ( !info )
        return;

    std::string lang = lang_string( ch, ch );

    set_char_color( info->color, ch );

    /* --- SELF --- */
    if ( info->self_fmt )
    {
        if ( info->uses_verb )
            ch_printf( ch, info->self_fmt, verb.c_str(), lang.c_str(), argument.c_str() );
        else
            ch_printf( ch, info->self_fmt, lang.c_str(), argument.c_str() );
    }

    /* --- OTHER --- */
    if ( info->other_fmt )
    {
        std::string tmp = info->uses_verb
            ? str_printf( info->other_fmt, verb.c_str() )
            : std::string( info->other_fmt );

        buf = info->prefix_fmt
            ? str_printf( "%s %s", info->prefix_fmt, tmp.c_str() )
            : tmp;
    }

    /* --- act() override --- */
    if ( info->use_act_to_char && !buf.empty() )
    {
        int position = ch->position;
        ch->position = POS_STANDING;

        act( info->color, buf, ch, argument, NULL, TO_CHAR );

        ch->position = position;
    }
}

void talk_channel( CHAR_DATA *ch, const std::string &argument_in, int channel, const std::string &verb )
{
    std::string argument = argument_in;
    std::string buf;
    std::string buf2;
    DESCRIPTOR_DATA *d;

    const CHANNEL_INFO *info = get_channel_info( channel );
    if ( !info )
        return;

    CLAN_DATA *clan = NULL;

    if ( !ch || !ch->in_room )
        return;

    if ( BV_IS_SET( ch->in_room->room_flags, ROOM_SILENCE ) )
    {
        send_to_char( "You can't do that here.\n", ch );
        return;
    }

    if ( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
    {
        if ( ch->master )
            send_to_char( "I don't think so...\n", ch->master );
        return;
    }

    if ( argument.empty() )
    {
        std::string tmp = str_printf( "%s what?\n", verb.c_str() );
        if ( !tmp.empty() )
            tmp[0] = UPPER( tmp[0] );
        send_to_char( tmp, ch );
        return;
    }

    if ( !IS_NPC( ch ) && BV_IS_SET( ch->act, PLR_SILENCE ) )
    {
        ch_printf( ch, "You can't %s.\n", verb.c_str() );
        return;
    }

    if ( info->requires_comlink && !has_comlink( ch ) )
    {
        send_to_char( "You need a comlink to do that!\n", ch );
        return;
    }

    if ( info->clan_only && !IS_NPC( ch ) && ch->pcdata )
        clan = ch->pcdata->clan;

    /* Apply drunk speech if needed */
    if ( info->use_drunk )
        argument = drunk_speech( argument, ch );

    BV_SET_BIT( ch->channels, channel );

    format_channel_message( ch, channel, verb, argument, buf );

    if ( BV_IS_SET( ch->in_room->room_flags, ROOM_LOGSPEECH ) )
    {
        buf2 = str_printf( "%s: %s (%s)",
            IS_NPC( ch ) ? ch->short_descr : ch->name,
            argument.c_str(),
            verb.c_str() );
        append_to_file( LOG_FILE, buf2.c_str() );
    }

    for ( d = first_descriptor; d; d = d->next )
    {
        CHAR_DATA *och = d->original ? d->original : d->character;
        CHAR_DATA *vch = d->character;

        if ( d->connected != CON_PLAYING )
            continue;

        if ( !vch || vch == ch )
            continue;

        if ( !BV_IS_SET( och->channels, channel ) )
            continue;

        if ( !can_receive_channel( ch, vch, och, info, clan ) )
            continue;

        std::string sbuf = argument;

        if ( !info->no_scramble &&
            !knows_language( vch, ch->speaking, ch ) &&
            ( !IS_NPC( ch ) || ch->speaking != 0 ) )
        {
            sbuf = scramble( argument, ch->speaking );
        }

        int position = vch->position;
        if ( !info->room_only )
            vch->position = POS_STANDING;

        MOBtrigger = FALSE;

        act( info->color, buf, ch, sbuf, vch, TO_VICT );

        vch->position = position;
    }
}

void to_channel( const std::string &argument, int channel, const std::string &verb, sh_int level )
{
    DESCRIPTOR_DATA *d;

    if ( !first_descriptor || argument.empty() )
        return;

    std::string buf = str_printf( "%s: %s\n", verb.c_str(), argument.c_str() );

    for ( d = first_descriptor; d; d = d->next )
    {
        CHAR_DATA *och = d->original ? d->original : d->character;
        CHAR_DATA *vch = d->character;

        if ( !och || !vch )
            continue;

        if ( ( !IS_IMMORTAL(vch) && channel != CHANNEL_ARENA )
        ||   ( vch->top_level < d->game->get_sysdata()->build_level && channel == CHANNEL_BUILD )
        ||   ( vch->top_level < d->game->get_sysdata()->log_level
        &&     ( channel == CHANNEL_LOG || channel == CHANNEL_COMM ) ) )
            continue;

        if ( d->connected == CON_PLAYING
        &&   BV_IS_SET( och->channels, channel )
        &&   vch->top_level >= level )
        {
            set_char_color( AT_LOG, vch );
            send_to_char( buf, vch );  // or send_to_char(buf, vch) if you have overload
        }
    }
}


void do_chat( CHAR_DATA *ch, char *argument )
{
    if ( ch->gold < 1 )
    {
    	send_to_char("&RYou don't have enough credits!\n",ch);
    	return;
    }
    
    ch->gold -= 1;
    
    talk_channel( ch, argument, CHANNEL_CHAT, "chat" );
    return;
}

void do_shiptalk( CHAR_DATA *ch, char *argument )
{
     SHIP_DATA *ship;

     if ( (ship = ship_from_cockpit(ch->game, ch->in_room->vnum)) == NULL )  
     {
    	  send_to_char("&RYou must be in the cockpit of a ship to do that!\n",ch);
    	  return;
     }
     talk_channel( ch, argument, CHANNEL_SHIP, "shiptalk" );
     return;
}    	        

void do_systemtalk( CHAR_DATA *ch, char *argument )
{
     SHIP_DATA *ship;
     
     if ( (ship = ship_from_cockpit(ch->game, ch->in_room->vnum)) == NULL )  
     {
    	  send_to_char("&RYou must be in the cockpit of a ship to do that!\n",ch);
    	  return;
     }
     talk_channel( ch, argument, CHANNEL_SYSTEM, "systemtalk" );
     return;
}    	        

void do_spacetalk( CHAR_DATA *ch, char *argument )
{
     SHIP_DATA *ship;

     if ( (ship = ship_from_cockpit(ch->game, ch->in_room->vnum)) == NULL )  
     {
    	  send_to_char("&RYou must be in the cockpit of a ship to do that!\n",ch);
    	  return;
     }
     talk_channel( ch, argument, CHANNEL_SPACE, "spacetalk" );
     return;
}    	        

void do_ooc( CHAR_DATA *ch, char *argument )
{
    if ( ch && !IS_NPC(ch) && ch->pcdata && ch->pcdata->bestowments && is_name("noooc", ch->pcdata->bestowments) )
    {
      send_to_char( "You have lost the ability to use that channel\n",ch);
      return;
    }

    talk_channel( ch, argument, CHANNEL_OOC, "ooc" );
    return;
}

void do_newbieasst( CHAR_DATA *ch, char *argument )
{
if ( !( ch->pcdata->bestowments && is_name( "newbieasst", ch->pcdata->bestowments) ))
           return;
    talk_channel( ch, argument, CHANNEL_NEWBIEASST, "newbieasst" );
    return;
}

void do_allclantalk( CHAR_DATA *ch, char *argument )
{
    if (NOT_AUTHED(ch))
    {
      send_to_char("Huh?\n", ch);
      return;
    }

    if ( IS_NPC( ch ) || !ch->pcdata->clan )
    {
	send_to_char( "Huh?\n", ch );
	return;
    }
    talk_channel( ch, argument, CHANNEL_ALLCLAN, "allclantalk" );
    return;
}

void do_clantalk( CHAR_DATA *ch, char *argument )
{
    if (NOT_AUTHED(ch))
    {
      send_to_char("Huh?\n", ch);
      return;
    }

    if ( IS_NPC( ch ) || !ch->pcdata->clan )
    {
	send_to_char( "Huh?\n", ch );
	return;
    }
    talk_channel( ch, argument, CHANNEL_CLAN, "clantalk" );
    return;
}

void do_newbiechat( CHAR_DATA *ch, char *argument )
{
    if ( ch->top_level > 5 )
    {
	send_to_char( "Aren't you a little old for the newbie channel?\n", ch );
	return;
    }
    talk_channel( ch, argument, CHANNEL_NEWBIE, "newbiechat" );
    return;
}

void do_ot( CHAR_DATA *ch, char *argument )
{
  do_ordertalk( ch, argument );
}

void do_ordertalk( CHAR_DATA *ch, char *argument )
{
      send_to_char("Huh?\n", ch);
      return;
}

void do_guildtalk( CHAR_DATA *ch, char *argument )
{
      send_to_char("Huh?\n", ch);
      return;
}

void do_music( CHAR_DATA *ch, char *argument )
{
    if (NOT_AUTHED(ch))
    {
      send_to_char("Huh?\n", ch);
      return;
    }
    talk_channel( ch, argument, CHANNEL_MUSIC, "sing" );
    return;
}


void do_quest( CHAR_DATA *ch, char *argument )
{
    if (NOT_AUTHED(ch))
    {
      send_to_char("Huh?\n", ch);
      return;
    }
    talk_channel( ch, argument, CHANNEL_QUEST, "quest" );
    return;
}

void do_ask( CHAR_DATA *ch, char *argument )
{
    if (NOT_AUTHED(ch))
    {
      send_to_char("Huh?\n", ch);
      return;
    }
    talk_channel( ch, argument, CHANNEL_ASK, "ask" );
    return;
}



void do_answer( CHAR_DATA *ch, char *argument )
{
    if (NOT_AUTHED(ch))
    {
      send_to_char("Huh?\n", ch);
      return;
    }
    talk_channel( ch, argument, CHANNEL_ASK, "answer" );
    return;
}



void do_shout( CHAR_DATA *ch, char *argument )
{
    if (NOT_AUTHED(ch))
    {
      send_to_char("Huh?\n", ch);
      return;
    }
  talk_channel( ch, drunk_speech( argument, ch ), CHANNEL_SHOUT, "shout" );
  WAIT_STATE( ch, 12 );
  return;
}



void do_yell( CHAR_DATA *ch, char *argument )
{
    if (NOT_AUTHED(ch))
    {
      send_to_char("Huh?\n", ch);
      return;
    }
  talk_channel( ch, drunk_speech( argument, ch ), CHANNEL_YELL, "yell" );
  return;
}



void do_immtalk( CHAR_DATA *ch, char *argument )
{
    if (NOT_AUTHED(ch))
    {
      send_to_char("Huh?\n", ch);
      return;
    }
    talk_channel( ch, argument, CHANNEL_IMMTALK, "immtalk" );
    return;
}


void do_i103( CHAR_DATA *ch, char *argument )
{
    if (NOT_AUTHED(ch))
    {
      send_to_char("Huh?\n", ch);
      return;
    }
    talk_channel( ch, argument, CHANNEL_103, "i103" );
    return;
}

void do_i104( CHAR_DATA *ch, char *argument )
{
    if (NOT_AUTHED(ch))
    {
      send_to_char("Huh?\n", ch);
      return;
    }
    talk_channel( ch, argument, CHANNEL_104, "i104" );
    return;
}

void do_i105( CHAR_DATA *ch, char *argument )
{
    if (NOT_AUTHED(ch))
    {
      send_to_char("Huh?\n", ch);
      return;
    }
    talk_channel( ch, argument, CHANNEL_105, "i105" );
    return;
}


void do_avtalk( CHAR_DATA *ch, char *argument )
{
    if (NOT_AUTHED(ch))
    {
      send_to_char("Huh?\n", ch);
      return;
    }
    talk_channel( ch, drunk_speech( argument, ch ), CHANNEL_AVTALK, "avtalk" );
    return;
}


void do_say(CHAR_DATA *ch, char *argument)
{
    std::string buf;
    CHAR_DATA *vch;
    FLAG_SET actflags;
    int lang;

    if (argument[0] == '\0')
    {
        send_to_char("Say what?\n", ch);
        return;
    }

    if (!ch || !ch->in_room)
        return;

    if (BV_IS_SET(ch->in_room->room_flags, ROOM_SILENCE))
    {
        send_to_char("You can't do that here.\n", ch);
        return;
    }

    actflags = ch->act;
    if (IS_NPC(ch))
        BV_REMOVE_BIT(ch->act, ACT_SECRETIVE);

    for (vch = ch->in_room->first_person; vch; vch = vch->next_in_room)
    {
        std::string sbuf = argument;
        std::string sbuflang;

        if (vch == ch)
            continue;

        if (!knows_language(vch, ch->speaking, ch) &&
            (!IS_NPC(ch) || ch->speaking != LANG_UNKNOWN))
            sbuf = scramble(argument, ch->speaking);

        sbuf = drunk_speech(sbuf, ch);

        MOBtrigger = FALSE;

        lang = ch->speaking;

        if (!IS_NPC(vch) &&
            knows_language(vch, ch->speaking, ch) &&
            (!IS_NPC(ch) || ch->speaking != LANG_UNKNOWN))
        {
            sbuflang = str_printf("$n says in %s, '$t'",
                    capitalize(get_flag_name(lang_names, lang, LANG_MAX)).c_str());
            act(AT_SAY, sbuflang, ch, sbuf, vch, TO_VICT);
        }
        else
        {
            act(AT_SAY, "$n says, in some language, '$t'",
                ch, sbuf, vch, TO_VICT);
        }
    }

    ch->act = actflags;

    MOBtrigger = FALSE;

    lang = ch->speaking;

    act(AT_SAY, "You say in $l '$T'",
        ch, NULL, drunk_speech(argument, ch), TO_CHAR);

    if (BV_IS_SET(ch->in_room->room_flags, ROOM_LOGSPEECH))
    {
        buf = str_printf("%s: %s",
                IS_NPC(ch) ? ch->short_descr : ch->name,
                argument);
        append_to_file(LOG_FILE, buf);
    }

    mprog_speech_trigger(argument, ch);
    if (char_died(ch))
        return;

    oprog_speech_trigger(argument, ch);
    if (char_died(ch))
        return;

    rprog_speech_trigger(argument, ch);
}



void do_tell( CHAR_DATA *ch, char *argument )
{
    std::string arg;
    std::string buf;
    std::string sbuf = argument;
    std::string sbuflang;
    CHAR_DATA *victim;
    int position;
    CHAR_DATA *switched_victim;
    bool ch_comlink;
    bool victim_comlink;
    OBJ_DATA *obj;
    CHAR_DATA *vch;
    bool sameroom = FALSE;
    
    switched_victim = NULL;

    if ( !BV_IS_SET( ch->channels, CHANNEL_TELLS ) 
    && !IS_IMMORTAL( ch ) )
    {
      act( AT_PLAIN, "You have tells turned off... try chan +tells first", ch, NULL, NULL,
		TO_CHAR );
      return;
    }

    if ( BV_IS_SET( ch->in_room->room_flags, ROOM_SILENCE ) )
    {
	send_to_char( "You can't do that here.\n", ch );
	return;
    }

    if (!IS_NPC(ch)
    && ( BV_IS_SET(ch->act, PLR_SILENCE)
    ||   BV_IS_SET(ch->act, PLR_NO_TELL) ) )
    {
	send_to_char( "You can't do that.\n", ch );
	return;
    }

    std::string argumentstr = one_argument( argument, arg );

    if ( arg.empty() || argumentstr.empty() )
    {
	send_to_char( "Tell whom what?\n", ch );
	return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL 
       || ( IS_NPC(victim) && victim->in_room != ch->in_room ) 
       || (!NOT_AUTHED(ch) && NOT_AUTHED(victim) && !IS_IMMORTAL(ch) ) )
    {
	send_to_char( "They can't hear you.\n", ch );
	return;
    }

    if ( ch == victim )
    {
	send_to_char( "You have a nice little chat with yourself.\n", ch );
	return;
    }
    
    if (victim->in_room == ch->in_room )
      sameroom = TRUE;
    
    if (victim->in_room != ch->in_room && !IS_IMMORTAL(ch) )
    {
      ch_comlink = FALSE;
      victim_comlink = FALSE;
    
      for ( obj = ch->last_carrying; obj; obj = obj->prev_content )
      {
        if (obj->pIndexData->item_type == ITEM_COMLINK)
        ch_comlink = TRUE;
      }
    
      if ( !ch_comlink )
      {
	send_to_char( "You need a comlink to do that!\n", ch);
	return;
      }
      
      if ( IS_IMMORTAL ( victim ) )
         victim_comlink = TRUE; 
      
      for ( obj = victim->last_carrying; obj; obj = obj->prev_content )
      {
        if (obj->pIndexData->item_type == ITEM_COMLINK)
        victim_comlink = TRUE;
      }

      if ( !victim_comlink )
      {
	send_to_char( "They don't seem to have a comlink!\n", ch);
	return;
      }
    
    }    
    
    if (NOT_AUTHED(ch) && !NOT_AUTHED(victim) && !IS_IMMORTAL(victim) )
    {
	send_to_char( "They can't hear you because you are not authorized.\n", ch);
	return;
    }

    if ( !IS_NPC( victim ) && ( victim->switched ) 
	&& ( get_trust( ch ) > LEVEL_AVATAR ) 
        && !BV_IS_SET(victim->switched->act, ACT_POLYMORPHED)
	&& !IS_AFFECTED(victim->switched, AFF_POSSESS) )
    {
      send_to_char( "That player is switched.\n", ch );
      return;
    }

   else if ( !IS_NPC( victim ) && ( victim->switched ) 
        && (BV_IS_SET(victim->switched->act, ACT_POLYMORPHED) 
 	||  IS_AFFECTED(victim->switched, AFF_POSSESS) ) )
     switched_victim = victim->switched;

   else if ( !IS_NPC( victim ) && ( !victim->desc ) )
    {
      send_to_char( "That player is link-dead.\n", ch );
      return;
    }

    if ( !BV_IS_SET( victim->channels, CHANNEL_TELLS ) 
    && ( !IS_IMMORTAL( ch ) || ( get_trust( ch ) < get_trust( victim ) ) ) )
    {
      act( AT_PLAIN, "They can't hear you.", ch, NULL, victim,
		TO_CHAR );
      return;
    }

    if ( !IS_NPC (victim) && ( BV_IS_SET (victim->act, PLR_SILENCE ) ) )
      {
      send_to_char( "That player is silenced.  They will receive your message but can not respond.\n", ch );
      }   

    if ( (!IS_IMMORTAL(ch) && !IS_AWAKE(victim) )
    || (!IS_NPC(victim) && BV_IS_SET(victim->in_room->room_flags, ROOM_SILENCE ) ) )
    {
    act( AT_PLAIN, "$E can't hear you.", ch, 0, victim, TO_CHAR );
	return;
    }

    if ( victim->desc		/* make sure desc exists first  -Thoric */
    &&   victim->desc->connected == CON_EDITING 
    &&   get_trust(ch) < LEVEL_GOD )
    {
	act( AT_PLAIN, "$E is currently in a writing buffer.  Please try again in a few minutes.", ch, 0, victim, TO_CHAR );
        return;
    }

    if ( !IS_NPC (victim) && ( BV_IS_SET (victim->act, PLR_AFK ) ) )
    {
      send_to_char( "That player is afk so he may not respond.\n", ch );
    }


    if(switched_victim)
      victim = switched_victim;
   
    act( AT_TELL, "(&COutgoing Message&B) ($l) $N: '$t'", ch, argumentstr, victim, TO_CHAR );
    position		= victim->position;
    victim->position	= POS_STANDING;

    sbuf = argumentstr;
    if ( !knows_language(victim, ch->speaking, ch) &&
       (!IS_NPC(ch) || ch->speaking != 0) )
      sbuf = scramble(argumentstr, ch->speaking);
    sbuf = drunk_speech( sbuf, ch );
    
    sbuflang = str_printf("((&CIncoming Message&B)(&C%s&B) $n: '$t'", lang_string(ch, victim ).c_str() );
	act( AT_TELL, sbuflang, ch, sbuf, victim, TO_VICT );


/*  if ( knows_language( victim, ch->speaking, ch )
    ||  (IS_NPC(ch) && !ch->speaking) )
	act( AT_TELL, "(&CIncoming Message&B) $n: '$t'", ch, argument, victim, TO_VICT );
    else
	act( AT_TELL, "(&CIncoming Message&B) $n: '$t'", ch, scramble(argument, ch->speaking), victim, TO_VICT ); */
    victim->position	= position;
    victim->reply	= ch;
    if (ch != NULL && victim != NULL && !IS_NPC(ch) && !IS_NPC(victim))	
      ch->retell          = victim;
    if ( BV_IS_SET( ch->in_room->room_flags, ROOM_LOGSPEECH ) )
    {
	    buf = str_printf("%s: %s (tell to) %s.",
		 IS_NPC( ch ) ? ch->short_descr : ch->name,
		 argument,
		 IS_NPC( victim ) ? victim->short_descr : victim->name );
	    append_to_file( LOG_FILE, buf );
    }
   if( !IS_IMMORTAL(ch) && !sameroom )
   {
/*    act( AT_ACTION, "$n appears to be quietly speaking to someone.", ch, NULL, NULL, TO_ROOM); */

    for ( vch = ch->in_room->first_person; vch; vch = vch->next_in_room )
    {
	if ( vch == ch )
	  continue;
	sbuf = argumentstr;
	if ( !knows_language(vch, ch->speaking, ch) &&
		 (!IS_NPC(ch) || ch->speaking != 0) )
	 sbuf = scramble(argumentstr, ch->speaking);
	sbuf = drunk_speech( sbuf, ch );

	MOBtrigger = FALSE;
    if ( ( !IS_NPC(vch) && knows_language(vch, ch->speaking, ch ) ) && ( (!IS_NPC(ch) ) || ch->speaking != 0 ) )
    {
      sbuflang = str_printf( "$n says quietly into $s comlink in %s '$t'", lang_string(ch, vch ).c_str() );
      act( AT_TELL, sbuflang, ch, sbuf, vch, TO_VICT );
    }
    else
    {
      act( AT_TELL, "$n says quietly into $s comlink in some language '$t'", ch, sbuf, vch, TO_VICT );
    }
    
//  act( AT_SAY, "$n says quietly into $s comlink '$t'", ch, sbuf, vch, TO_VICT );
    }
    if ( !IS_IMMORTAL(victim) )
      act( AT_ACTION, "$n's comlink buzzes with a message.", victim, NULL, NULL, TO_ROOM);
   }
   mprog_speech_trigger( argument, ch );
   return;
}



void do_reply( CHAR_DATA *ch, char *argument )
{
    std::string buf;
    std::string sbuf;
    std::string sbuflang;
    CHAR_DATA *victim;
    int position;
    CHAR_DATA *vch;
    bool sameroom = FALSE;

    BV_SET_BIT( ch->channels, CHANNEL_TELLS );
    if ( BV_IS_SET( ch->in_room->room_flags, ROOM_SILENCE ) )
    {
	send_to_char( "You can't do that here.\n", ch );
	return;
    }

    if ( !IS_NPC(ch) && BV_IS_SET(ch->act, PLR_SILENCE) )
    {
	send_to_char( "Your message didn't get through.\n", ch );
	return;
    }

    if ( ( victim = ch->reply ) == NULL )
    {
	send_to_char( "They can't hear you.\n", ch );
	return;
    }

    if ( !IS_NPC( victim ) && ( victim->switched )
	&& can_see( ch, victim ) && ( get_trust( ch ) > LEVEL_AVATAR ) )
    {
      send_to_char( "That player is switched.\n", ch );
      return;
    }
   else if ( !IS_NPC( victim ) && ( !victim->desc ) )
    {
      send_to_char( "That player is link-dead.\n", ch );
      return;
    }

    if ( ( !BV_IS_SET( victim->channels, CHANNEL_TELLS ) )
    && ( !IS_IMMORTAL( ch ) || ( get_trust( ch ) < get_trust( victim ) ) ) )
    {
      act( AT_PLAIN, "They can't hear you.", ch, NULL, victim,
	TO_CHAR );
      return;
    }

    if ( ( !IS_IMMORTAL(ch) && !IS_AWAKE(victim) )
    || ( !IS_NPC(victim) && BV_IS_SET( victim->in_room->room_flags, ROOM_SILENCE ) ) )
    {
    act( AT_PLAIN, "$E can't hear you.", ch, 0, victim, TO_CHAR );
	return;
    }

    if ( !IS_NPC (victim) && ( BV_IS_SET (victim->act, PLR_AFK ) ) )
    {
      send_to_char( "That player is afk so he may not respond.\n", ch );
    }
    
    if (victim->in_room == ch->in_room )
      sameroom = TRUE;

    act( AT_TELL, "(&COutgoing Message&B)($l) $N: '$t'", ch, argument, victim, TO_CHAR );
    position		= victim->position;
    victim->position	= POS_STANDING;

    sbuf = argument;
    if ( !knows_language(victim, ch->speaking, ch) &&
       (!IS_NPC(ch) || ch->speaking != 0) )
      sbuf = scramble(argument, ch->speaking);
    sbuf = drunk_speech( sbuf, ch );

    sbuflang = str_printf("(&CIncoming Message&B)(&C%s&B) $n: '$t'", lang_string(ch, victim ).c_str() );
    act( AT_TELL, sbuflang, ch, sbuf, victim, TO_VICT );

    victim->position	= position;
    victim->reply	= ch;
    if ( BV_IS_SET( ch->in_room->room_flags, ROOM_LOGSPEECH ) )
    {
	buf = str_printf("%s: %s (reply to) %s.",
		 IS_NPC( ch ) ? ch->short_descr : ch->name,
		 argument,
		 IS_NPC( victim ) ? victim->short_descr : victim->name );
	append_to_file( LOG_FILE, buf );
    }

   if( !IS_IMMORTAL(ch) && !sameroom )
   {
/*    act( AT_ACTION, "$n appears to be quietly speaking to someone.", ch, NULL, NULL, TO_ROOM); */

    for ( vch = ch->in_room->first_person; vch; vch = vch->next_in_room )
    {
	if ( vch == ch )
	  continue;
	sbuf = argument;
	if ( !knows_language(vch, ch->speaking, ch) &&
		 (!IS_NPC(ch) || ch->speaking != 0) )
	 sbuf = scramble(argument, ch->speaking);
	sbuf = drunk_speech( sbuf, ch );

	MOBtrigger = FALSE;

    if ( ( !IS_NPC(vch) && knows_language(vch, ch->speaking, ch ) ) && ( (!IS_NPC(ch) ) || ch->speaking != 0 ) )
    {
      sbuflang = str_printf( "$n says quietly into $s comlink in %s '$t'", lang_string(ch, vch ).c_str() );
      act( AT_TELL, sbuflang, ch, sbuf, vch, TO_VICT );
    }
    else
    {
      act( AT_TELL, "$n says quietly into $s comlink in some language '$t'", ch, sbuf, vch, TO_VICT );
    }
    
//	act( AT_SAY, "$n says quietly into his comlink '$t'", ch, sbuf, vch, TO_VICT );
    }
    if ( !IS_IMMORTAL(victim) )
      act( AT_ACTION, "$n's comlink buzzes with a message.", victim, NULL, NULL, TO_ROOM);
   }

    return;
}


void do_retell( CHAR_DATA *ch, char *argument )
{	
	CHAR_DATA *victim;
	int position;	
	bool sameroom = FALSE;	
	std::string buf;
	std::string sbuf;
	std::string sbuflang;
	if( argument[0] == '\0' ) {
		send_to_char("Retell what?\n",ch );		
		return;	
	}
	BV_SET_BIT( ch->channels, CHANNEL_TELLS );
	if( BV_IS_SET( ch->in_room->room_flags, ROOM_SILENCE ) )	{
		send_to_char("You can't do that here.\n",ch );
		return;	
	}
	if( !IS_NPC(ch) && BV_IS_SET( ch->act, PLR_SILENCE ) )	{
		send_to_char("You've been forbidden to send tells.\n",ch);
		return;	
	}
	if( ( victim = ch->retell ) == NULL )	{
		send_to_char("They can't hear you.\n",ch);
		return;	
	}
	if( !IS_NPC( victim ) && !victim->desc )	{
		send_to_char("That player is link-dead.\n",ch);
		return;	
	}
	if( !BV_IS_SET( victim->channels, CHANNEL_TELLS ) &&
		( !IS_IMMORTAL(ch) || get_trust( ch ) < get_trust( victim) ) )	{
		send_to_char("They can't hear you.\n",ch);
		return;	
	}
	if( (!IS_IMMORTAL(ch) && !IS_AWAKE( ch ) ) ||
		(!IS_NPC(victim) && BV_IS_SET( victim->in_room->room_flags, ROOM_SILENCE ) ) )	{
		act( AT_PLAIN, "$E can't hear you.", ch, NULL, victim, TO_CHAR );
		return;	
	}
	if( !IS_NPC(victim) && BV_IS_SET( victim->act, PLR_AFK ) )
		send_to_char("They are afk so they may not respond.\n",ch );
	if( victim->in_room == ch->in_room ) 
		sameroom = TRUE;
	act( AT_TELL, "(&COutgoing Tell&B) ($l) $N: '$t'", ch, argument, victim, TO_CHAR );
	position = victim->position;	
	victim->position = POS_STANDING;	

        sbuf = argument;
        if ( !knows_language(victim, ch->speaking, ch) &&
           (!IS_NPC(ch) || ch->speaking != 0) )
          sbuf = scramble(argument, ch->speaking);
        sbuf = drunk_speech( sbuf, ch );

        sbuflang = str_printf("(&CIncoming Message&B)(&C%s&B) $n: '$t'", lang_string(ch, victim ).c_str() );
        act( AT_TELL, sbuflang, ch, sbuf, victim, TO_VICT );

	victim->position = position;	
	victim->reply = ch;
	if( BV_IS_SET( ch->in_room->room_flags, ROOM_LOGSPEECH ) )	{
		buf = str_printf("%s: %s (retell to) %s", IS_NPC( ch ) ?ch->short_descr : ch->name, argument, IS_NPC( victim ) ? victim->short_descr : victim->name );
		append_to_file( LOG_FILE, buf );	
	}	
	if(!IS_IMMORTAL( ch ) && !sameroom )	{
		CHAR_DATA *vch;
		for( vch = ch->in_room->first_person; vch != NULL; vch = vch->next_in_room )
		{			
			MOBtrigger = FALSE;

		        sbuf = argument;
		        if ( !knows_language(victim, ch->speaking, ch) &&
		           (!IS_NPC(ch) || ch->speaking != 0) )
		          sbuf = scramble(argument, ch->speaking);
		        sbuf = drunk_speech( sbuf, ch );

		    if ( ( !IS_NPC(vch) && knows_language(vch, ch->speaking, ch ) ) && ( (!IS_NPC(ch) ) || ch->speaking != 0 ) )
		    {
		      sbuflang = str_printf("$n says quietly into $s comlink in %s '$t'", lang_string(ch, vch ).c_str() );
		      act( AT_TELL, sbuflang, ch, sbuf, vch, TO_VICT );
		    }
		    else
		    {
		      act( AT_TELL, "$n says quietly into $s comlink in some language '$t'", ch, sbuf, vch, TO_VICT );
		    }
		}		
		if( !IS_IMMORTAL(victim) )
			act( AT_ACTION, "$n's comlink buzzes with a message.", victim, NULL, NULL, TO_ROOM );
	}	
	return;
}


void do_emote( CHAR_DATA *ch, char *argument )
{
    std::string buf;
    char *plast;
    FLAG_SET actflags;

    if ( !IS_NPC(ch) && BV_IS_SET(ch->act, PLR_NO_EMOTE) )
    {
	send_to_char( "You can't show your emotions.\n", ch );
	return;
    }

    if ( argument[0] == '\0' )
    {
	send_to_char( "Emote what?\n", ch );
	return;
    }

    actflags = ch->act;
    if ( IS_NPC( ch ) ) BV_REMOVE_BIT( ch->act, ACT_SECRETIVE );
    for ( plast = argument; *plast != '\0'; plast++ )
	;

    buf = argument;
    if ( isalpha(plast[-1]) )
	buf += ".";

    MOBtrigger = FALSE;
    act( AT_ACTION, "$n $T", ch, NULL, buf, TO_ROOM );
    MOBtrigger = FALSE;
    act( AT_ACTION, "$n $T", ch, NULL, buf, TO_CHAR );
    ch->act = actflags;
    if (!ch || !ch->in_room) return;
    if ( BV_IS_SET( ch->in_room->room_flags, ROOM_LOGSPEECH ) )
    {
	    buf = str_printf("%s %s (emote)", IS_NPC( ch ) ? ch->short_descr : ch->name,
		 argument );
	append_to_file( LOG_FILE, buf );
    }
    return;
}


void do_bug( CHAR_DATA *ch, char *argument )
{
    if( !argument || argument[0] == '\0' )
    {
        send_to_char( "What do you want to submit as a bug?\n", ch );
        return;
    }    
    append_file( ch, BUG_FILE, argument );
    send_to_char( "Ok.  Thanks.\n", ch );
    return;
}


void do_ide( CHAR_DATA *ch, char *argument )
{
    send_to_char("If you want to send an idea, type 'idea <message>'.\n", ch);
    send_to_char("If you want to identify an object and have the identify spell,\n", ch);
    send_to_char("Type 'cast identify <object>'.\n", ch);
    return;
}

void do_idea( CHAR_DATA *ch, char *argument )
{
    if( !argument || argument[0] == '\0' )
    {
        send_to_char( "What do you want to submit as an idea?\n", ch );
        return;
    }
    append_file( ch, IDEA_FILE, argument );
    send_to_char( "Ok.  Thanks.\n", ch );
    return;
}



void do_typo( CHAR_DATA *ch, char *argument )
{
    if( !argument || argument[0] == '\0' )
    {
        send_to_char( "What do you want to submit as a typo?\n", ch );
        return;
    }
    append_file( ch, TYPO_FILE, argument );
    send_to_char( "Ok.  Thanks.\n", ch );
    return;
}



/*  Moved to space2.c for rental ships
void do_rent( CHAR_DATA *ch, char *argument )
{
    set_char_color( AT_WHITE, ch );
    send_to_char( "There is no rent here.  Just save and quit.\n", ch );
    return;
}
*/ 


void do_qui( CHAR_DATA *ch, char *argument )
{
    set_char_color( AT_RED, ch );
    send_to_char( "If you want to QUIT, you have to spell it out.\n", ch );
    return;
}

void do_quit( CHAR_DATA *ch, char *argument )
{
/*  OBJ_DATA *obj; */ /* Unused */
    int x, y, cost;
//  int level;
    std::string qbuf;
    std::string buf;

    if ( IS_NPC(ch) && BV_IS_SET(ch->act, ACT_POLYMORPHED))
    { 
      send_to_char("You can't quit while polymorphed.\n", ch);
      return;
    }

    if ( IS_NPC(ch) )
	return;

    if ( ch->position == POS_FIGHTING )
    {
	set_char_color( AT_RED, ch );
	send_to_char( "No way! You are fighting.\n", ch );
	return;
    }

    if ( ch->position  < POS_STUNNED  )
    {
	set_char_color( AT_BLOOD, ch );
	send_to_char( "You're not DEAD yet.\n", ch );
	return;
    }

    if ( auction->item != NULL && ((ch == auction->buyer) || (ch == auction->seller) ) )
    {
	send_to_char("Wait until you have bought/sold the item on auction.\n", ch);
	return;
    }
        if (!ch || !ch->in_room) return;
    if ( !IS_IMMORTAL(ch) && ch->in_room && (!BV_IS_SET( ch->in_room->room_flags , ROOM_HOTEL ) && !BV_IS_SET( ch->in_room->room_flags, ROOM_PLR_HOME ) ) && !NOT_AUTHED(ch) )
    {
	send_to_char("You may not quit here.\n", ch);
	send_to_char("You will have to find a safer resting place such as a hotel...\n", ch);
	send_to_char("Maybe you could HAIL a speeder.\n", ch);
	return;
    }
    
    if ( !IS_IMMORTAL(ch) && ch->in_room && BV_IS_SET( ch->in_room->room_flags, ROOM_HOTEL ) && !BV_IS_SET( ch->in_room->room_flags, ROOM_PLR_HOME ) && 
        !BV_IS_SET( ch->in_room->room_flags, ROOM_SPACECRAFT )&& !NOT_AUTHED(ch) )
    {
    	cost = get_cost_quit( ch );
	if( !cost )
	{
	  buf = "The keeper takes a good look at you and adopts a look of pity, letting you stay here for free\n";
	  send_to_char("The keeper takes a good look at you and adopts a look of pity, letting you stay here for free\n", ch);
	}
    	else if( ch->gold < cost )
        {
	  buf = str_printf("You need %d credits to spend the night here!\n", cost );
	  send_to_char(buf, ch);
	  return;
        }
        else
        {
	  buf = str_printf("The keeper takes a good look at you and lets you stay here for %d credits\n", cost );
	  send_to_char(buf, ch);
          ch->gold -= cost;
          if( ch->in_room && ch->in_room->area )
            boost_economy( ch->in_room->area, cost );
        }
     }
    
    if ( ch->challenged )
    {
      qbuf = str_printf("%s has quit! Challenge is void. WHAT A WUSS!",ch->name);
      ch->challenged=NULL;
      to_channel(qbuf,CHANNEL_ARENA,"&RArena&W",5);
    }

    set_char_color( AT_WHITE, ch );
    send_to_char( "Your surroundings begin to fade as a mystical swirling vortex of colors\nenvelops your body... When you come to, things are not as they were.\n\n", ch );
    act( AT_SAY, "A strange voice says, 'We await your return, $n...'", ch, NULL, NULL, TO_CHAR );
    act( AT_BYE, "$n has left the game.", ch, NULL, NULL, TO_ROOM );
    set_char_color( AT_GREY, ch);

    if (ch->in_room)
      log_printf_plus( LOG_COMM, ch->top_level, "%s has quit. (Room %d)",ch->name, ch->in_room->vnum);
    else
      log_printf_plus( LOG_COMM, ch->top_level, "%s has quit.", ch->name );
    quitting_char = ch;
    save_char_obj( ch );
    save_home(ch);

    if ( ch->pcdata->pet )
    {
       act( AT_BYE, "$N follows $S master out of the game.", ch, NULL, 
		ch->pcdata->pet, TO_ROOM );
       extract_char( ch->pcdata->pet, TRUE );
    }

    saving_char = NULL;

//  level = get_trust(ch);
    /*
     * After extract_char the ch is no longer valid!
     */
    extract_char( ch, TRUE );
    for ( x = 0; x < MAX_WEAR; x++ )
	for ( y = 0; y < MAX_LAYERS; y++ )
	    save_equipment[x][y] = NULL;

    return;
}


static void send_file_to_descriptor( DESCRIPTOR_DATA *d, const std::string& filename )
{
    FILE *fp;
    int c;
    std::string buff;

    if ( !d || filename.empty() )
        return;

    if ( ( fp = fopen( filename.c_str(), "r" ) ) == NULL )
        return;

    buff.reserve( MAX_STRING_LENGTH * 2 );

    while ( ( c = fgetc( fp ) ) != EOF )
        buff.push_back( (char)c );

    FCLOSE( fp );
    output_to_descriptor( d, buff );
}

void send_rip_screen( CHAR_DATA *ch )
{
    if ( ch && ch->desc )
        send_file_to_descriptor( ch->desc, RIPSCREEN_FILE );
}

void send_rip_title( CHAR_DATA *ch )
{
    if ( ch && ch->desc )
        send_file_to_descriptor( ch->desc, RIPTITLE_FILE );
}

void send_ansi_title( CHAR_DATA *ch )
{
    if ( ch && ch->desc )
        send_file_to_descriptor( ch->desc, ANSITITLE_FILE );
}

void send_ascii_title( CHAR_DATA *ch )
{
    if ( ch && ch->desc )
        send_file_to_descriptor( ch->desc, ASCTITLE_FILE );
}


void do_rip( CHAR_DATA *ch, char *argument )
{
    std::string arg;

    one_argument( argument, arg );

    if ( arg.empty() )
    {
        send_to_char( "Rip ON or OFF?\n", ch );
        return;
    }
        if ( (!str_cmp(arg,"on")) || (!str_cmp(arg,"ON")) ) {
        send_rip_screen(ch);
        BV_SET_BIT(ch->act,PLR_ANSI);
        return;
    }

    if ( (!str_cmp(arg,"off")) || (!str_cmp(arg,"OFF")) ) {
        send_to_char( "!|*\nRIP now off...\n", ch );
        return;
    }
}

void do_ansi( CHAR_DATA *ch, char *argument )
{
    std::string arg;

    one_argument( argument, arg );

    if ( arg.empty() )
    {
	send_to_char( "ANSI ON or OFF?\n", ch );
	return;
    }
        if ( (!str_cmp(arg,"on")) || (!str_cmp(arg,"ON")) ) {
        BV_SET_BIT(ch->act,PLR_ANSI);
        set_char_color( AT_WHITE + AT_BLINK, ch);
        send_to_char( "ANSI ON!!!\n", ch);
        return;
    }

    if ( (!str_cmp(arg,"off")) || (!str_cmp(arg,"OFF")) ) {
        BV_REMOVE_BIT(ch->act,PLR_ANSI);
        send_to_char( "Okay... ANSI support is now off\n", ch );
        return;
    }
}

void do_sound( CHAR_DATA *ch, char *argument )
{
    std::string arg;

    one_argument( argument, arg );

    if ( arg.empty() )
    {
        send_to_char( "SOUND ON or OFF?\n", ch );
        return;
    }
    if ( (!str_cmp(arg,"on")) || (!str_cmp(arg,"ON")) ) {
        BV_SET_BIT(ch->act,PLR_SOUND);
        set_char_color( AT_WHITE + AT_BLINK, ch);
        send_to_char( "SOUND ON!!!\n", ch);
        send_to_char( "!!SOUND(hopeknow)", ch);
        return;
    }

    if ( (!str_cmp(arg,"off")) || (!str_cmp(arg,"OFF")) ) {
        BV_REMOVE_BIT(ch->act,PLR_SOUND);
        send_to_char( "Okay... SOUND support is now off\n", ch );
        return;
    }
}

void do_save( CHAR_DATA *ch, char *argument )
{
    if ( IS_NPC(ch) && BV_IS_SET(ch->act, ACT_POLYMORPHED))
    { 
      send_to_char("You can't save while polymorphed.\n", ch);
      return;
    }

    if ( IS_NPC(ch) )
	return;

    ch->affected_by != race_table[ch->race].affected;
//    if ( !BV_IS_SET( ch->affected_by, race_table[ch->race].affected ) )
	    //BV_SET_BIT( ch->affected_by, race_table[ch->race].affected );
    ch->resistant != race_table[ch->race].resist;
    ch->susceptible != race_table[ch->race].suscept;

    if ( NOT_AUTHED(ch) )
    { 
      send_to_char("You can't save untill after you've graduated from the academy.\n", ch);
      return;
    }

    save_char_obj( ch );
    save_home (ch );
    if (!ch || !ch->in_room) return;
    if ( BV_IS_SET( ch->in_room->room_flags, ROOM_CLANSTOREROOM ) )
    	save_storeroom( ch->in_room );
    
    saving_char = NULL;
    send_to_char( "Ok.\n", ch );
    return;
}


/*
 * Something from original DikuMUD that Merc yanked out.
 * Used to prevent following loops, which can cause problems if people
 * follow in a loop through an exit leading back into the same room
 * (Which exists in many maze areas)			-Thoric
 */
bool circle_follow( CHAR_DATA *ch, CHAR_DATA *victim )
{
    CHAR_DATA *tmp;

    for ( tmp = victim; tmp; tmp = tmp->master )
	if ( tmp == ch )
	  return TRUE;
    return FALSE;
}
void do_dismiss( CHAR_DATA *ch, char *argument )
{
   std::string arg;
   CHAR_DATA *victim;

    one_argument( argument, arg );     

    if ( arg.empty() )
    {
	send_to_char( "Dismiss whom?\n", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
        send_to_char( "They aren't here.\n", ch );
        return;
    }

    if ( ( IS_AFFECTED( victim, AFF_CHARM ) )
    && ( IS_NPC( victim ) )
   && ( victim->master == ch ) )
    {
	stop_follower( victim );
        stop_hating( victim );
        stop_hunting( victim );
         stop_fearing( victim );
        act( AT_ACTION, "$n dismisses $N.", ch, NULL, victim, TO_NOTVICT );
 	act( AT_ACTION, "You dismiss $N.", ch, NULL, victim, TO_CHAR );
    }
    else
    {
	send_to_char( "You cannot dismiss them.\n", ch );
    }
  return;
}


void do_follow( CHAR_DATA *ch, char *argument )
{
    std::string arg;
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Follow whom?\n", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n", ch );
	return;
    }

    if ( IS_AFFECTED(ch, AFF_CHARM) && ch->master )
    {
	act( AT_PLAIN, "But you'd rather follow $N!", ch, NULL, ch->master, TO_CHAR );
	return;
    }

    if ( victim == ch )
    {
	if ( !ch->master )
	{
	    send_to_char( "You already follow yourself.\n", ch );
	    return;
	}
	stop_follower( ch );
	return;
    }

    if ( circle_follow( ch, victim ) )
    {
	send_to_char( "Following in loops is not allowed... sorry.\n", ch );
	return;
    }

    if ( ch->master )
	stop_follower( ch );

    add_follower( ch, victim );
    return;
}



void add_follower( CHAR_DATA *ch, CHAR_DATA *master )
{
    if ( ch->master )
    {
        bug( "%s: non-null master.", __func__ );
	    return;
    }

    ch->master        = master;
    ch->leader        = NULL;
    if ( IS_NPC(ch) && BV_IS_SET(ch->act, ACT_PET) && !IS_NPC(master) )
	master->pcdata->pet = ch;

    if ( can_see( master, ch ) )
    act( AT_ACTION, "$n now follows you.", ch, NULL, master, TO_VICT );

    act( AT_ACTION, "You now follow $N.",  ch, NULL, master, TO_CHAR );

    return;
}



void stop_follower( CHAR_DATA *ch )
{
    if ( !ch->master )
    {
      bug( "%s: null master.", __func__ );
	    return;
    }

    if ( IS_NPC(ch) && !IS_NPC(ch->master) && ch->master->pcdata->pet == ch )
	ch->master->pcdata->pet = NULL;

    if ( IS_AFFECTED(ch, AFF_CHARM) )
    {
	BV_REMOVE_BIT( ch->affected_by, AFF_CHARM );
	affect_strip( ch, gsn_charm_person );
    }

    if ( can_see( ch->master, ch ) )
    act( AT_ACTION, "$n stops following you.",     ch, NULL, ch->master, TO_VICT    );
    act( AT_ACTION, "You stop following $N.",      ch, NULL, ch->master, TO_CHAR    );

    ch->master = NULL;
    ch->leader = NULL;

    return;
}



void die_follower( CHAR_DATA *ch )
{
    CHAR_DATA *fch;

    if ( ch->master )
	stop_follower( ch );

    ch->leader = NULL;

    for ( fch = first_char; fch; fch = fch->next )
    {
	if ( fch->master == ch )
	    stop_follower( fch );
	if ( fch->leader == ch )
	    fch->leader = fch;
    }
    return;
}



void do_order( CHAR_DATA *ch, char *argument )
{
    std::string arg;
    std::string argumentstr = argument;
    CHAR_DATA *victim;
    CHAR_DATA *och;
    CHAR_DATA *och_next;
    bool found;
    bool fAll;

    argumentstr = one_argument( argumentstr, arg );

    if ( arg.empty() || argumentstr.empty() )
    {
	send_to_char( "Order whom to do what?\n", ch );
	return;
    }

    if ( IS_AFFECTED( ch, AFF_CHARM ) )
    {
	send_to_char( "You feel like taking, not giving, orders.\n", ch );
	return;
    }

    if ( !str_cmp( arg, "all" ) )
    {
	fAll   = TRUE;
	victim = NULL;
    }
    else
    {
	fAll   = FALSE;
	if ( ( victim = get_char_room( ch, arg ) ) == NULL )
	{
	    send_to_char( "They aren't here.\n", ch );
	    return;
	}

	if ( victim == ch )
	{
	    send_to_char( "Aye aye, right away!\n", ch );
	    return;
	}

	if ( !IS_AFFECTED(victim, AFF_CHARM) || victim->master != ch )
	{
	    send_to_char( "Do it yourself!\n", ch );
	    return;
	}
    }

    if ( !str_prefix("mp",argumentstr) )
    {
        send_to_char( "But that's cheating!\n", ch );
        return;
    }

    found = FALSE;
    if (!ch || !ch->in_room) return;
    for ( och = ch->in_room->first_person; och; och = och_next )
    {
	och_next = och->next_in_room;

      if( IS_AFFECTED( och, AFF_CHARM ) && och->master == ch && ( fAll || och == victim ) && !IS_IMMORTAL( och ) )
	{
	    found = TRUE;
	    act( AT_ACTION, "$n orders you to '$t'.", ch, argumentstr, och, TO_VICT );
        interpret(och, argumentstr);
	}
    }

    if ( found )
    {
        log_printf_plus( LOG_NORMAL, ch->top_level,
                "%s: order %s.", ch->name, argumentstr.data() );
 	    send_to_char( "Ok.\n", ch );
        WAIT_STATE( ch, 12 );
    }
    else
	send_to_char( "You have no followers here.\n", ch );
    return;
}

void do_group( CHAR_DATA *ch, char *argument )
{
    std::string arg;
    CHAR_DATA *victim = NULL;

    one_argument( argument, arg );

    if ( arg.empty() )
    {
	CHAR_DATA *gch;

/*	CHAR_DATA *leader;

	leader = ch->leader ? ch->leader : ch;*/
	set_char_color( AT_GREEN, ch );
	ch_printf( ch, "%s's group:\n", PERS(ch, ch) );

/* Changed so that no info revealed on possess */
	for ( gch = first_char; gch; gch = gch->next )
	{
	    if ( is_same_group( gch, ch ) )
	    {
		set_char_color( AT_DGREEN, ch );
		if (IS_AFFECTED(gch, AFF_POSSESS))
		  ch_printf( ch,
		    "[%2d %s] %-16s %4s/%4s hp %4s/%4s mv %5s xp\n",
		    gch->top_level,
		    IS_NPC(gch) ? "Mob" : race_table[gch->race].race_name,
		    capitalize( PERS(gch, ch) ).c_str(),
		    "????",   
		    "????",
		    "????",
		    "????",
		    "?????"    );

		else
		  ch_printf( ch,
		    "[%2d %s] %-16s %4d/%4d hp %4d/%4d mv\n",
		    gch->top_level,
		    IS_NPC(gch) ? "Mob" : race_table[gch->race].race_name,
		    capitalize( PERS(gch, ch) ).c_str(),
		    gch->hit,
		    gch->max_hit,
		    gch->move,  
		    gch->max_move   );
	    }
	}
	return;
    }

    if ( !str_cmp( arg, "disband" ))
    {
	CHAR_DATA *gch;
	int count = 0;

	if ( ch->leader || ch->master )
	{
	    send_to_char( "You cannot disband a group if you're following someone.\n", ch );
	    return;
	}
	
	for ( gch = first_char; gch; gch = gch->next )
	{
	    if ( is_same_group( ch, gch )
	    && ( ch != gch ) )
	    {
		gch->leader = NULL;
		gch->master = NULL;
		count++;
		send_to_char( "Your group is disbanded.\n", gch );
	    }
	}

	if ( count == 0 )
	   send_to_char( "You have no group members to disband.\n", ch );
	else
	   send_to_char( "You disband your group.\n", ch );

    return;
    }

    if ( !str_cmp( arg, "all" ) )
    {
        CHAR_DATA *rch;
        int count = 0;

            for ( rch = ch->in_room->first_person; rch; rch = rch->next_in_room )
            {
                if ( ch != rch
                &&   !IS_NPC( rch )
                &&   rch->master == ch
                &&   !ch->master
                &&   !ch->leader
                &&   !is_same_group( rch, ch )
                )
                {
                    rch->leader = ch;
                    count++;
                }
            }
        
        if ( count == 0 )
            send_to_char( "You have no eligible group members.\n", ch );
        else
        {
        act( AT_ACTION, "$n groups $s followers.", ch, NULL, victim, TO_ROOM );
        send_to_char( "You group your followers.\n", ch );
        }
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n", ch );
	return;
    }

    if ( ch->master || ( ch->leader && ch->leader != ch ) )
    {
	send_to_char( "But you are following someone else!\n", ch );
	return;
    }

    if ( victim->master != ch && ch != victim )
    {
    act( AT_PLAIN, "$N isn't following you.", ch, NULL, victim, TO_CHAR );
	return;
    }

    if ( is_same_group( victim, ch ) && ch != victim )
    {
	victim->leader = NULL;
    act( AT_ACTION, "$n removes $N from $s group.",   ch, NULL, victim, TO_NOTVICT );
    act( AT_ACTION, "$n removes you from $s group.",  ch, NULL, victim, TO_VICT    );
    act( AT_ACTION, "You remove $N from your group.", ch, NULL, victim, TO_CHAR    );
	return;
    }

    victim->leader = ch;
    act( AT_ACTION, "$N joins $n's group.", ch, NULL, victim, TO_NOTVICT );
    act( AT_ACTION, "You join $n's group.", ch, NULL, victim, TO_VICT    );
    act( AT_ACTION, "$N joins your group.", ch, NULL, victim, TO_CHAR    );
    return;
}



/*
 * 'Split' originally by Gnort, God of Chaos.
 */
void do_split( CHAR_DATA *ch, char *argument )
{
    std::string buf;
    std::string arg;
    CHAR_DATA *gch;
    int members;
    int amount;
    int share;
    int extra;

    one_argument( argument, arg );

    if ( arg.empty() )
    {
	send_to_char( "Split how much?\n", ch );
	return;
    }

    amount = strtoi( arg );

    if ( amount < 0 )
    {
	send_to_char( "Your group wouldn't like that.\n", ch );
	return;
    }

    if ( amount == 0 )
    {
	send_to_char( "You hand out zero credits, but no one notices.\n", ch );
	return;
    }

    if ( ch->gold < amount )
    {
	send_to_char( "You don't have that many credits.\n", ch );
	return;
    }

    members = 0;
    for ( gch = ch->in_room->first_person; gch; gch = gch->next_in_room )
    {
	if ( is_same_group( gch, ch ) )
	    members++;
    }

    
    if (( BV_IS_SET(ch->act, PLR_AUTOGOLD)) && (members < 2))
    return;

    if ( members < 2 )
    {
	send_to_char( "Just keep it all.\n", ch );
	return;
    }

    share = amount / members;
    extra = amount % members;

    if ( share == 0 )
    {
	send_to_char( "Don't even bother, cheapskate.\n", ch );
	return;
    }

    ch->gold -= amount;
    ch->gold += share + extra;

    set_char_color( AT_GOLD, ch );
    ch_printf( ch,
	"You split %d credits.  Your share is %d credits.\n",
	amount, share + extra );

    buf = str_printf("$n splits %d credits.  Your share is %d credits.",
	    amount, share );

    for ( gch = ch->in_room->first_person; gch; gch = gch->next_in_room )
    {
	if ( gch != ch && is_same_group( gch, ch ) )
	{
	    act( AT_GOLD, buf, ch, NULL, gch, TO_VICT );
	    gch->gold += share;
	}
    }
    return;
}



void do_gtell( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *gch;

    if ( argument[0] == '\0' )
    {
	send_to_char( "Tell your group what?\n", ch );
	return;
    }

    if ( BV_IS_SET( ch->act, PLR_NO_TELL ) )
    {
	send_to_char( "Your message didn't get through!\n", ch );
	return;
    }

    /*
     * Note use of send_to_char, so gtell works on sleepers.
     */
    for ( gch = first_char; gch; gch = gch->next )
    {
	if ( is_same_group( gch, ch ) )
	{
	    set_char_color( AT_GTELL, gch );
	    /* Groups unscrambled regardless of clan language.  Other languages
		   still garble though. -- Altrag */
	    if ( knows_language( gch, ch->speaking, gch )
	    ||  (IS_NPC(ch) && !ch->speaking) )
		    ch_printf( gch, "%s tells the group '%s'.\n", ch->name, argument );
	    else
		    ch_printf( gch, "%s tells the group '%s'.\n", ch->name, scramble(argument, ch->speaking) );
	}
    }

    return;
}


/*
 * It is very important that this be an equivalence relation:
 * (1) A ~ A
 * (2) if A ~ B then B ~ A
 * (3) if A ~ B  and B ~ C, then A ~ C
 */
bool is_same_group( CHAR_DATA *ach, CHAR_DATA *bch )
{
    if ( ach->leader ) ach = ach->leader;
    if ( bch->leader ) bch = bch->leader;
    return ach == bch;
}

/*
 * this function sends raw argument over the AUCTION: channel
 * I am not too sure if this method is right..
 */

void talk_auction (const std::string& argument)
{
    DESCRIPTOR_DATA *d;
    CHAR_DATA *original;
    std::string buf = argument;

    for (d = first_descriptor; d; d = d->next)
    {
        original = d->original ? d->original : d->character; /* if switched */
        if ((d->connected == CON_PLAYING) && BV_IS_SET(original->channels,CHANNEL_AUCTION) 
        && !BV_IS_SET(original->in_room->room_flags, ROOM_SILENCE) && !NOT_AUTHED(original))
            act( AT_GOSSIP, buf, original, "", NULL, TO_CHAR );
    }
}

/*
 * Language support functions. -- Altrag
 * 07/01/96
 * Replaced for new bitset 3-29-26 DV
 */

void init_language_sn(GameContext *game)
{
    for (int i = 0; i < LANG_MAX; ++i)
    {
        const char *name = get_flag_name(lang_names, i, LANG_MAX);

        if (!name)
        {
            lang_sn[i] = -1;
            continue;
        }

        lang_sn[i] = skill_lookup(name);

        if (lang_sn[i] < 0)
        {
            bug("%s: No skill found for language %d (%s)", __func__, i, name);
        }
    }
}

bool knows_language(CHAR_DATA *ch, int language, CHAR_DATA *cch)
{
    if (!ch)
        return false;

    // Immortals know everything
    if (!IS_NPC(ch) && IS_IMMORTAL(ch))
        return true;

    // NPCs with no language restriction know everything
    if (IS_NPC(ch) && !ch->speaks.any())
        return true;
    // Verify language is not out of bounds
    if (language < 0 || language >= LANG_MAX)
        return false;
    if (language == LANG_DIVINE) // Divine language can be understood by all - replacing speak all
        return true;        

    // Clan language handling
    if (language == LANG_CLAN)
    {
        if (IS_NPC(ch) || IS_NPC(cch))
            return true;

        return (ch->pcdata->clan &&
                ch->pcdata->clan == cch->pcdata->clan);
    }

    // NPCs: simple BitSet check
    if (IS_NPC(ch))
        return BV_IS_SET(ch->speaks, language);

    // PCs: racial language (single enum)
    if (race_table[ch->race].language == language)
        return true;

    // If player doesn't know the language at all
    if (!BV_IS_SET(ch->speaks, language))
        return false;

    // Skill-based comprehension
        int sn = lang_sn[language];

    if (sn != -1)
        return (number_percent() - 1 < ch->pcdata->learned[sn]);

    return false;
}

bool can_learn_lang(CHAR_DATA *ch, int language)
{
    if (!ch)
        return false;

    // Clan language cannot be learned
    if (language == LANG_CLAN)
        return false;

    // NPCs and immortals don't learn
    if (IS_NPC(ch) || IS_IMMORTAL(ch))
        return false;

    // Racial language cannot be learned
    if (race_table[ch->race].language == language)
        return false;
    if (language == LANG_DIVINE)
        return false;
    // Must be a valid language
    if (!VALID_LANG(language))
        return false;

    // If already known, check if maxed
    if (BV_IS_SET(ch->speaks, language))
    {
        int sn = lang_sn[language];

        if (sn < 0)
        {
            bug("%s: valid language without sn: %d", __func__, language);
            return false;
        }

        if (ch->pcdata->learned[sn] >= 99)
            return false;
    }

    return true;
}

/*
int const lang_array[] = { LANG_COMMON, LANG_WOOKIEE, LANG_TWI_LEK, LANG_RODIAN,
						   LANG_HUTT, LANG_MON_CALAMARI, LANG_SHISTAVANEN, LANG_EWOK,
						   LANG_ITHORIAN, LANG_GOTAL, LANG_DEVARONIAN,
						   LANG_DROID, LANG_SPIRITUAL, LANG_MAGICAL,
						   LANG_GAMORREAN, LANG_GOD, LANG_ANCIENT, LANG_JAWA,
						   LANG_CLAN, LANG_ADARIAN, LANG_VERPINE, LANG_DEFEL,
						   LANG_TRANDOSHAN, LANG_CHADRA_FAN, LANG_QUARREN, 
						   LANG_SULLUSTAN, LANG_FALLEEN, LANG_BINARY, 
						   LANG_YEVETHAN, LANG_GAND, LANG_DUROS, LANG_COYNITE, LANG_UNKNOWN };
*/
/* Note: does not count racial language.  This is intentional (for now). */
int countlangs(const FLAG_SET &languages)
{
    int count = 0;

    for (size_t i = 0; i < LANG_MAX; ++i)
    {
        if (i == LANG_CLAN)
            continue;

        if (BV_IS_SET(languages, i))
            ++count;
    }

    return count;
}

const flag_name lang_names[] =
{
    { LANG_COMMON,              "common" },
    { LANG_WOOKIEE,             "shyriiwook" },
    { LANG_TWI_LEK,             "ryl" },
    { LANG_RODIAN,              "rodese" },
    { LANG_HUTT,                "huttese" },
    { LANG_MON_CALAMARI,        "mon calamari" },
    { LANG_SHISTAVANEN,         "shistavanen" },
    { LANG_EWOK,                "ewokese" },
    { LANG_ITHORIAN,            "ithorese" },
    { LANG_GOTAL,               "gotalse" },
    { LANG_DEVARONIAN,          "devaronese" },
    { LANG_BARABEL,             "barabel" },
    { LANG_FIRRERREO,           "firrerrean" },
    { LANG_BOTHAN,              "bothese" },
    { LANG_GAMORREAN,           "gamorrese" },
    { LANG_TOGORIAN,            "togorese" },
    { LANG_KUBAZ,               "kubazian" },
    { LANG_JAWA,                "jawaese" },
    { LANG_CLAN,	            "clan" },
    { LANG_ADARIAN,	            "adarian" },
    { LANG_VERPINE,	            "verpine" },
    { LANG_DEFEL,               "defel" },
    { LANG_TRANDOSHAN,          "dosh" },
    { LANG_CHADRA_FAN,          "chadra-fan" },
    { LANG_QUARREN,             "quarrenese" },
    { LANG_SULLUSTAN,           "sullustese" },
    { LANG_FALLEEN,             "falleen" },
    { LANG_BINARY,              "binary" },
    { LANG_YEVETHAN,            "yevethan" },
    { LANG_GAND,                "gand" },
    { LANG_DUROS,               "durese" },
    { LANG_COYNITE,             "coynite" },
    { LANG_NOGHRI,              "honoghran" },
    { LANG_DIVINE,	            "divine" },
    { LANG_ANCIENT,             "ancient" },
    { LANG_DROID,               "droidspeak" },
    { LANG_SPIRITUAL,           "spiritual" },
    { LANG_MAGICAL,             "magical" },
    { LANG_MAX,                 "Lang_Max" },

    { (size_t)-1, nullptr } // terminator
};

const struct flag_name channel_names[] =
{
    { CHANNEL_AUCTION,    "auction" },
    { CHANNEL_CHAT,       "chat" },
    { CHANNEL_QUEST,      "quest" },
    { CHANNEL_IMMTALK,    "immtalk" },
    { CHANNEL_MUSIC,      "music" },
    { CHANNEL_ASK,        "ask" },
    { CHANNEL_SHOUT,      "shout" },
    { CHANNEL_YELL,       "yell" },
    { CHANNEL_MONITOR,    "monitor" },
    { CHANNEL_LOG,        "log" },
    { CHANNEL_104,        "104" },
    { CHANNEL_CLAN,       "clan" },
    { CHANNEL_BUILD,      "build" },
    { CHANNEL_105,        "105" },
    { CHANNEL_AVTALK,     "avtalk" },
    { CHANNEL_PRAY,       "pray" },
    { CHANNEL_COUNCIL,    "council" },
    { CHANNEL_GUILD,      "guild" },
    { CHANNEL_COMM,       "comm" },
    { CHANNEL_TELLS,      "tells" },
    { CHANNEL_ORDER,      "order" },
    { CHANNEL_NEWBIE,     "newbie" },
    { CHANNEL_VULGAR,     "vulgar" },
    { CHANNEL_OOC,        "ooc" },
    { CHANNEL_SHIP,       "ship" },
    { CHANNEL_SYSTEM,     "system" },
    { CHANNEL_SPACE,      "space" },
    { CHANNEL_103,        "103" },
    { CHANNEL_ARENA,      "arena" },
    { CHANNEL_ALLCLAN,    "allclan" },
    { CHANNEL_NEWS,       "news" },
    { CHANNEL_NEWBIEASST, "newbieasst" },

    { CHANNEL_MAX,        NULL },
    { (size_t)-1, nullptr } // terminator    
};

void do_speak(CHAR_DATA *ch, char *argument)
{
    std::string arg;
    one_argument(argument, arg);

    // Immortal: speak all (keep behavior, though questionable now)
    if (!str_cmp(arg, "all") && IS_IMMORTAL(ch))
    {
        set_char_color(AT_SAY, ch);
        ch->speaking = LANG_DIVINE; // or whatever default you want
        send_to_char("Now speaking the divine language, understood by all.\n", ch);
        return;
    }

    if (!str_prefix(arg, "clan"))
    {
        set_char_color(AT_SAY, ch);
        send_to_char("Clan language? There is no such thing.\n", ch);
        return;
    }

    // --- Race restrictions (unchanged logic) ---
    if (ch->race == RACE_WOOKIEE && str_prefix(arg, "shyriiwook"))
    {
        set_char_color(AT_SAY, ch);
        send_to_char("Your vocal cords refuse to make those sounds.\n", ch);
        return;
    }

    if (ch->race == RACE_VERPINE && str_prefix(arg, "verpine"))
    {
        set_char_color(AT_SAY, ch);
        send_to_char("Your jaws cant pronounce that language.\n", ch);
        return;
    }

    if (ch->race == RACE_GAMORREAN && str_prefix(arg, "gamorrese"))
    {
        set_char_color(AT_SAY, ch);
        send_to_char("You can barely speak your own language!\n", ch);
        return;
    }

    if (!str_prefix(arg, "common") &&
        (ch->race == RACE_EWOK || ch->race == RACE_JAWA))
    {
        set_char_color(AT_SAY, ch);
        send_to_char("You can not speak common, although you may be able to learn to understand it.\n", ch);
        return;
    }

    if (!str_prefix(arg, "ryl") && ch->race != RACE_TWI_LEK)
    {
        set_char_color(AT_SAY, ch);
        send_to_char("To speak the Twi'lek language requires body parts that you don't have.\n", ch);
        return;
    }

    if (!str_prefix(arg, "binary") && !is_droid(ch))
    {
        set_char_color(AT_SAY, ch);
        send_to_char("Only droids are able to speak binary.\n", ch);
        return;
    }

    if (!str_prefix(arg, "verpine") && ch->race != RACE_VERPINE)
    {
        set_char_color(AT_SAY, ch);
        send_to_char("You need certain bodyparts you do not have to speak verpine.\n", ch);
        return;
    }

    if (!str_prefix(arg, "dosh") && ch->race != RACE_TRANDOSHAN)
    {
        set_char_color(AT_SAY, ch);
        send_to_char("Only a fellow reptile can speak the trandoshan language.\n", ch);
        return;
    }

    if (!str_prefix(arg, "gamorrese") && ch->race != RACE_GAMORREAN)
    {
        set_char_color(AT_SAY, ch);
        send_to_char("The gamorrean language can only be spoken by the pigs themselves!\n", ch);
        return;
    }

    if (!str_prefix(arg, "ithorese") && ch->race != RACE_ITHORIAN)
    {
        set_char_color(AT_SAY, ch);
        send_to_char("You can not replicate the sounds of the ithorian language.\n", ch);
        return;
    }

    // --- New language selection logic ---
    for (int lang = 0; lang < LANG_MAX; ++lang)
    {
        const std::string name = get_flag_name(lang_names, lang, LANG_MAX);
        if (name.empty())
            continue;

        if (!str_prefix(arg, name))
        {
            if (!knows_language(ch, lang, ch))
                break;

            if (lang == LANG_CLAN &&
                (IS_NPC(ch) || !ch->pcdata->clan))
                break;

            if (lang == LANG_DIVINE && !IS_IMMORTAL(ch))
            {
                set_char_color(AT_SAY, ch);
                send_to_char("You cannot speak the divine tongue.\n", ch);
                return;
            }
            ch->speaking = lang;

            set_char_color(AT_SAY, ch);
            ch_printf(ch, "You now speak %s.\n", name.c_str());
            return;
        }
    }

    set_char_color(AT_SAY, ch);
    send_to_char("You do not know that language.\n", ch);
}

void do_languages(CHAR_DATA *ch, char *argument)
{
    std::string arg;
    int lang;
    int sn;

    std::string argumentstr = one_argument(argument, arg);

    if (!arg.empty() && !str_prefix(arg, "learn") &&
        !IS_IMMORTAL(ch) && !IS_NPC(ch))
    {
        CHAR_DATA *sch;
        std::string arg2;
        int prct;

        argumentstr = one_argument(argumentstr, arg2);

        if (arg2.empty())
        {
            send_to_char("Learn which language?\n", ch);
            return;
        }

        // Find language
        for (lang = 0; lang < LANG_MAX; ++lang)
        {
            if (lang == LANG_CLAN)
                continue;

            if (!get_flag_name(lang_names, lang, LANG_MAX))
                continue;

            if (!str_prefix(arg2, get_flag_name(lang_names, lang, LANG_MAX)))
                break;
        }

        if (lang >= LANG_MAX)
        {
            send_to_char("That is not a language.\n", ch);
            return;
        }

        if (!VALID_LANG(lang))
        {
            send_to_char("You may not learn that language.\n", ch);
            return;
        }

        // Divine cannot be learned
        if (lang == LANG_DIVINE)
        {
            send_to_char("That language cannot be learned.\n", ch);
            return;
        }

        sn = lang_sn[lang];
        if (sn < 0)
        {
            send_to_char("That is not a language.\n", ch);
            return;
        }

        // Already fluent?
        if (race_table[ch->race].language == lang ||
            ch->pcdata->learned[sn] >= 99)
        {
            act(AT_PLAIN, "You are already fluent in $t.",
                ch, get_flag_name(lang_names, lang, LANG_MAX), NULL, TO_CHAR);
            return;
        }

        // Find a teacher
        for (sch = ch->in_room->first_person; sch; sch = sch->next)
        {
            if (IS_NPC(sch) && BV_IS_SET(sch->act, ACT_SCHOLAR) &&
                knows_language(sch, ch->speaking, ch) &&
                knows_language(sch, lang, sch) &&
                (!sch->speaking || knows_language(ch, sch->speaking, sch)))
                break;
        }

        if (!sch)
        {
            send_to_char("There is no one who can teach that language here.\n", ch);
            return;
        }

        if (ch->gold < 25)
        {
            send_to_char("language lessons cost 25 credits... you don't have enough.\n", ch);
            return;
        }

        ch->gold -= 25;

        // Learning progress
        prct = 5 + (get_curr_int(ch) / 6) + (get_curr_wis(ch) / 7);

        ch->pcdata->learned[sn] += prct;
        ch->pcdata->learned[sn] = UMIN(ch->pcdata->learned[sn], 99);

        BV_SET_BIT(ch->speaks, lang);

        if (ch->pcdata->learned[sn] == prct)
            act(AT_PLAIN, "You begin lessons in $t.", ch, get_flag_name(lang_names, lang, LANG_MAX), NULL, TO_CHAR);
        else if (ch->pcdata->learned[sn] < 60)
            act(AT_PLAIN, "You continue lessons in $t.", ch, get_flag_name(lang_names, lang, LANG_MAX), NULL, TO_CHAR);
        else if (ch->pcdata->learned[sn] < 60 + prct)
            act(AT_PLAIN, "You feel you can start communicating in $t.", ch, get_flag_name(lang_names, lang, LANG_MAX), NULL, TO_CHAR);
        else if (ch->pcdata->learned[sn] < 99)
            act(AT_PLAIN, "You become more fluent in $t.", ch, get_flag_name(lang_names, lang, LANG_MAX), NULL, TO_CHAR);
        else
            act(AT_PLAIN, "You now speak perfect $t.", ch, get_flag_name(lang_names, lang, LANG_MAX), NULL, TO_CHAR);

        return;
    }

    // Display known languages
    for (lang = 0; lang < LANG_MAX; ++lang)
    {
        if (!VALID_LANG(lang))
            continue;

        if (!get_flag_name(lang_names, lang, LANG_MAX))
            continue;

        if (ch->speaking == lang || (IS_NPC(ch) && ch->speaking == 0))
            set_char_color(AT_RED, ch);
        else
            set_char_color(AT_SAY, ch);

        sn = lang_sn[lang];

        if (sn < 0)
            send_to_char("(  0) ", ch);
        else
            ch_printf(ch, "(%3d) ", ch->pcdata->learned[sn]);

        send_to_char(get_flag_name(lang_names, lang, LANG_MAX), ch);
        send_to_char("\n", ch);
    }

    send_to_char("\n", ch);
}


void do_vulgar( CHAR_DATA *ch, char *argument )
{
    if (NOT_AUTHED(ch))
    {
      send_to_char("Huh?\n", ch);
      return;
    }
    talk_channel( ch, argument, CHANNEL_VULGAR, "war" );
    return;
}

void do_ahelp( CHAR_DATA *ch, char *argument )
{
    set_char_color( AT_PLAIN, ch );
    if ( argument[0] == '\0' )
    {
        send_to_char( "\nUsage:  'ahelp list' or 'ahelp clear now'\n", ch);
        return;
    }
    if ( !str_cmp( argument, "clear now" ) )
    {
        FILE *fp = fopen( HELP_FILE, "w" );
        if ( fp )
          FCLOSE( fp );
        send_to_char( "Add Help file cleared.\n", ch);
        return;
    }
    if ( !str_cmp( argument, "list" ) )
    {
	  send_to_char( "\n VNUM \n.......\n", ch );
        show_file( ch, HELP_FILE );
    }
    else
    {
        send_to_char( "\nUsage:  'ahelp list' or 'ahelp clear now'\n", ch);
        return;
    }
    return;
}

