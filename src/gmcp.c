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
*			 GMCP Handling			                                       *
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

/*
Additional things to add:
Character class/ability
Character configs
Character position/status
Character drug/addictions
Character languages
Character affect data

Combat data:
Target data (name, status)

Ships:
Ship status
Target status
System data
Hyperspace jump data

Character Skills
Character Items
Room data
People in room
objects in room
Ships in room

*/

void handle_gmcp(DESCRIPTOR_DATA *d, unsigned char *data, int len);
void send_gmcp(DESCRIPTOR_DATA *d, const std::string &msg);
void dispatch_gmcp(DESCRIPTOR_DATA *d, const std::string &module, const std::string &data);
void gmcp_core_ping(DESCRIPTOR_DATA *d, const std::string &data);
void gmcp_core_hello(DESCRIPTOR_DATA *d, const std::string &data);
void gmcp_sendf(DESCRIPTOR_DATA *d, const std::string &module, const char *fmt, ...);
void gmcp_send_char_vitals(DESCRIPTOR_DATA *d);
void gmcp_send_char_status(DESCRIPTOR_DATA *d);
void gmcp_send_room_info(DESCRIPTOR_DATA *d);
void gmcp_send_all(void (*func)(DESCRIPTOR_DATA *));
void gmcp_core_supports_set(DESCRIPTOR_DATA *d, const std::string &data);
void gmcp_core_supports_remove(DESCRIPTOR_DATA *d, const std::string &data);
void gmcp_core_supports_add(DESCRIPTOR_DATA *d, const std::string &data);
void gmcp_unsubscribe(DESCRIPTOR_DATA *d, const std::string &module);
void gmcp_subscribe(DESCRIPTOR_DATA *d, const std::string &module);
bool gmcp_is_subscribed(DESCRIPTOR_DATA *d, const std::string &module);
const char *gmcp_cache_get(DESCRIPTOR_DATA *d, const char *key);
const char *gmcp_cache_get(DESCRIPTOR_DATA *d, const std::string &key);

bool gmcp_cache_set(DESCRIPTOR_DATA *d, const char *key, const char *value);
bool gmcp_cache_set(DESCRIPTOR_DATA *d, const std::string &key, const std::string &value);
bool gmcp_cache_set(DESCRIPTOR_DATA *d, const std::string &key, const char *value);
bool gmcp_cache_set(DESCRIPTOR_DATA *d, const char *key, const std::string &value);

/* GMCP pending flags */
#define GMCP_PEND_VITALS    BV00
#define GMCP_PEND_ROOM      BV01
#define GMCP_PEND_STATUS    BV02
#define GMCP_PEND_STATS     BV03


/* === GMCP MODULE SYSTEM (NEW) =============================== */

typedef void (*gmcp_handler_f)(DESCRIPTOR_DATA *d, const std::string &data);

typedef struct
{
    const char *name;
    gmcp_handler_f handler;
} gmcp_module_t;

/* Forward declarations */
void gmcp_core_ping(DESCRIPTOR_DATA *d, const std::string &data);
void gmcp_core_hello(DESCRIPTOR_DATA *d, const std::string &data);
void dispatch_gmcp(DESCRIPTOR_DATA *d, const std::string &module, const std::string &data);

/* Module table */
static const gmcp_module_t gmcp_modules[] =
{
    { "Core.Ping",  gmcp_core_ping  },
    { "Core.Hello", gmcp_core_hello },

    /* future modules go here */
    { "Core.Supports.Set",  gmcp_core_supports_set },
    { "Core.Supports.Add",    gmcp_core_supports_add },
    { "Core.Supports.Remove", gmcp_core_supports_remove },    
    { NULL, NULL }
};
/* === GMCP JSON BUILDER ============================ */

typedef struct
{
    std::string buf;
    bool first;
} gmcp_json_t;

void gmcp_json_init(gmcp_json_t *j)
{
    j->buf = "{";
    j->first = true;
}

void gmcp_json_add_int(gmcp_json_t *j, const std::string &key, int value)
{
    if (!j->first)
        j->buf += ",";
    j->first = false;
    j->buf += "\"" + key + "\":" + std::to_string(value);
}

static std::string gmcp_escape_json(const std::string &s)
{
    std::string out;
    out.reserve(s.size());

    for (char c : s)
    {
        switch (c)
        {
            case '\\': out += "\\\\"; break;
            case '"':  out += "\\\""; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += c;      break;
        }
    }
    return out;
}

void gmcp_json_add_str(gmcp_json_t *j, const std::string &key, const std::string &val)
{
    if (!j->first)
        j->buf += ",";
    j->first = false;

    j->buf += "\"";
    j->buf += key;
    j->buf += "\":\"";
    j->buf += gmcp_escape_json(val);
    j->buf += "\"";
}

const char *gmcp_cache_get(DESCRIPTOR_DATA *d, const char *key)
{
    if (!d || !key)
        return NULL;

    for (int i = 0; i < d->gmcp_cache.count; i++)
    {
        if (!strcmp(d->gmcp_cache.entries[i].key, key))
            return d->gmcp_cache.entries[i].value;
    }
    return NULL;
}

const char *gmcp_cache_get(DESCRIPTOR_DATA *d, const std::string &key)
{
    return gmcp_cache_get(d, key.c_str());
}

const std::string &gmcp_json_close(gmcp_json_t *j)
{
    if (j->buf.empty() || j->buf.back() != '}')
        j->buf += "}";
    return j->buf;
}

void gmcp_json_add_cache_int(gmcp_json_t *j, DESCRIPTOR_DATA *d,
                            const char *json_key, const char *cache_key)
{
    const char *val = gmcp_cache_get(d, cache_key);
    if (val)
        gmcp_json_add_int(j, json_key, atoi(val));
}

void gmcp_json_add_cache_str(gmcp_json_t *j, DESCRIPTOR_DATA *d,
                            const char *json_key, const char *cache_key)
{
    const char *val = gmcp_cache_get(d, cache_key);
    if (val)
        gmcp_json_add_str(j, json_key, val);
}

void build_char_vitals(gmcp_json_t *j, DESCRIPTOR_DATA *d)
{
    gmcp_json_add_cache_int(j, d, "hp", "Char.Vitals.hp");
    gmcp_json_add_cache_int(j, d, "maxhp", "Char.Vitals.maxhp");
    if ( d->character && (d->character->skill_level[FORCE_ABILITY] > 1 || IS_IMMORTAL(d->character)) )
    {
        gmcp_json_add_cache_int(j, d, "force", "Char.Vitals.mana");
        gmcp_json_add_cache_int(j, d, "maxforce", "Char.Vitals.maxmana");
    }
    gmcp_json_add_cache_int(j, d, "move", "Char.Vitals.move");
    gmcp_json_add_cache_int(j, d, "maxmove", "Char.Vitals.maxmove");
    gmcp_json_add_cache_int(j, d, "credits", "Char.Vitals.credits");
    gmcp_json_add_cache_int(j, d, "bank", "Char.Vitals.bank");
    gmcp_json_add_cache_int(j, d, "pkills", "Char.Vitals.pkills");
    gmcp_json_add_cache_int(j, d, "mkills", "Char.Vitals.mkills");
    
   
}

void build_room_info(gmcp_json_t *j, DESCRIPTOR_DATA *d)
{
    gmcp_json_add_cache_int(j, d, "num",  "Room.Info.num");
    gmcp_json_add_cache_str(j, d, "name", "Room.Info.name");
    gmcp_json_add_cache_int(j, d, "rentship",  "Room.Info.rentship");
    gmcp_json_add_cache_int(j, d, "serinstop",  "Room.Info.serinstop");


}

void build_char_status(gmcp_json_t *j, DESCRIPTOR_DATA *d)
{
    gmcp_json_add_cache_str(j, d, "name", "Char.Status.name");
    gmcp_json_add_cache_str(j, d, "fullname", "Char.Status.fullname");
    gmcp_json_add_cache_str(j, d, "class", "Char.Status.class");
    gmcp_json_add_cache_str(j, d, "race", "Char.Status.race");    
    gmcp_json_add_cache_int(j, d, "wimpy",      "Char.Status.wimpy");
    gmcp_json_add_cache_str(j, d, "wanted",      "Char.Status.wanted");

    gmcp_json_add_cache_str(j, d, "clan", "Char.Status.clan");    
    gmcp_json_add_cache_str(j, d, "clan salary", "Char.Status.salary");    
    gmcp_json_add_cache_str(j, d, "clan kills", "Char.Status.pkills");    
    gmcp_json_add_cache_str(j, d, "clan deaths", "Char.Status.pdeaths");    

    if (d->character && IS_IMMORTAL(d->character))
    {
        gmcp_json_add_cache_int(j, d, "trust", "Char.Status.trust"); 
        gmcp_json_add_cache_int(j, d, "wizinvis", "Char.Status.wizinvis");    
        gmcp_json_add_cache_str(j, d, "bamfin", "Char.Status.Bamfin");    
        gmcp_json_add_cache_str(j, d, "bamfout", "Char.Status.Bamfout");    
        if (d->character->pcdata->area)
        {
            gmcp_json_add_cache_str(j, d, "arealoaded", "Char.Status.AreaLoaded");    
            gmcp_json_add_cache_int(j, d, "lrvnum", "Char.Status.LRVnum");    
            gmcp_json_add_cache_int(j, d, "hrvnum", "Char.Status.HRVnum");    
            gmcp_json_add_cache_int(j, d, "lovnum", "Char.Status.LOVnum");    
            gmcp_json_add_cache_int(j, d, "hovnum", "Char.Status.HOVnum");    
            gmcp_json_add_cache_int(j, d, "lmvnum", "Char.Status.LMVnum");    
            gmcp_json_add_cache_int(j, d, "hmvnum", "Char.Status.HMVnum");    

        }
    }    

}

void build_char_stats(gmcp_json_t *j, DESCRIPTOR_DATA *d)
{
    gmcp_json_add_cache_int(j, d, "str",        "Char.Stats.str");
    gmcp_json_add_cache_int(j, d, "dex",        "Char.Stats.dex");
    gmcp_json_add_cache_int(j, d, "con",        "Char.Stats.con");
    gmcp_json_add_cache_int(j, d, "int",        "Char.Stats.int");    
    gmcp_json_add_cache_int(j, d, "wis",        "Char.Stats.wis");    
    gmcp_json_add_cache_int(j, d, "cha",        "Char.Stats.cha");    
    gmcp_json_add_cache_int(j, d, "hitroll",    "Char.Stats.hitroll");    
    gmcp_json_add_cache_int(j, d, "damroll",    "Char.Stats.damroll");    
    gmcp_json_add_cache_int(j, d, "armor",      "Char.Stats.armor");    
    gmcp_json_add_cache_int(j, d, "align",      "Char.Stats.align");    
    gmcp_json_add_cache_int(j, d, "weight",      "Char.Stats.weight");    
    gmcp_json_add_cache_int(j, d, "maxweight",      "Char.Stats.maxweight");    
    gmcp_json_add_cache_int(j, d, "numitems",      "Char.Stats.numitems");    
    gmcp_json_add_cache_int(j, d, "maxnitems",      "Char.Stats.maxnumitems");    


}

void gmcp_send_from_cache(DESCRIPTOR_DATA *d, const char *module,
                          void (*builder)(gmcp_json_t *, DESCRIPTOR_DATA *))
{
    gmcp_json_t j;
    gmcp_json_init(&j);

    builder(&j, d);

    gmcp_sendf(d, module, "%s", gmcp_json_close(&j).c_str());
}

int gmcp_cache_get_int(DESCRIPTOR_DATA *d, const char *key)
{
    const char *v = gmcp_cache_get(d, key);
    return v ? atoi(v) : 0;
}

bool gmcp_cache_set(DESCRIPTOR_DATA *d, const char *key, const char *value)
{
    if (!d || !key || !value)
        return false;

    for (int i = 0; i < d->gmcp_cache.count; i++)
    {
        if (!strcmp(d->gmcp_cache.entries[i].key, key))
        {
            if (!strcmp(d->gmcp_cache.entries[i].value, value))
                return false; /* no change */

            strncpy(d->gmcp_cache.entries[i].value, value, 127);
            d->gmcp_cache.entries[i].value[127] = '\0';
            return true; /* changed */
        }
    }

    if (d->gmcp_cache.count >= GMCP_MAX_CACHE)
        return false;

    strncpy(d->gmcp_cache.entries[d->gmcp_cache.count].key, key, 63);
    d->gmcp_cache.entries[d->gmcp_cache.count].key[63] = '\0';

    strncpy(d->gmcp_cache.entries[d->gmcp_cache.count].value, value, 127);
    d->gmcp_cache.entries[d->gmcp_cache.count].value[127] = '\0';

    d->gmcp_cache.count++;
    return true;
}

bool gmcp_cache_set(DESCRIPTOR_DATA *d, const std::string &key, const std::string &value)
{
    return gmcp_cache_set(d, key.c_str(), value.c_str());
}

bool gmcp_cache_set(DESCRIPTOR_DATA *d, const std::string &key, const char *value)
{
    return gmcp_cache_set(d, key.c_str(), value);
}

bool gmcp_cache_set(DESCRIPTOR_DATA *d, const char *key, const std::string &value)
{
    return gmcp_cache_set(d, key, value.c_str());
}

bool gmcp_cache_set_int(DESCRIPTOR_DATA *d, const char *key, int value)
{
    return gmcp_cache_set(d, key, std::to_string(value));
}

void gmcp_cache_clear_prefix(DESCRIPTOR_DATA *d, const char *prefix)
{
    for (int i = 0; i < d->gmcp_cache.count; )
    {
        if (!strncmp(d->gmcp_cache.entries[i].key, prefix, strlen(prefix)))
        {
            d->gmcp_cache.entries[i] =
                d->gmcp_cache.entries[d->gmcp_cache.count - 1];
            d->gmcp_cache.count--;
        }
        else
        {
            i++;
        }
    }
}

bool gmcp_has(DESCRIPTOR_DATA *d, const char *name)
{
    for (int i = 0; i < d->gmcp_cap_count; i++)
    {
        if (!strcmp(d->gmcp_caps[i].name, name))
            return true;
    }
    if (d->gmcp_cap_count == 0)
        return true;
    return false;
}

void handle_gmcp(DESCRIPTOR_DATA *d, unsigned char *data, int len)
{
    if (len <= 0)
        return;

    std::string msg(reinterpret_cast<const char *>(data), len);

    fprintf(stderr, "GMCP RECV: %s\r\n", msg.c_str());

    const size_t pos = msg.find(' ');

    if (pos != std::string::npos)
        dispatch_gmcp(d, msg.substr(0, pos), msg.substr(pos + 1));
    else
        dispatch_gmcp(d, msg, "");
}

void dispatch_gmcp(DESCRIPTOR_DATA *d, const std::string &module, const std::string &data)
{
    for (int i = 0; gmcp_modules[i].name != NULL; i++)
    {
        if (module == gmcp_modules[i].name)
        {
            gmcp_modules[i].handler(d, data);
            return;
        }
    }

    if (module.compare(0, 9, "External.") == 0 ||
        module.compare(0, 7, "Client.") == 0)
        return;

    fprintf(stderr, "GMCP: unhandled module %s\r\n", module.c_str());
}

static std::string gmcp_trim_token(const std::string &s)
{
    const std::string trim_chars = "[]\" ,";
    const size_t start = s.find_first_not_of(trim_chars);
    if (start == std::string::npos)
        return "";

    const size_t end = s.find_last_not_of(trim_chars);
    return s.substr(start, end - start + 1);
}

static bool gmcp_extract_json_string(const std::string &data,
                                     const std::string &key,
                                     std::string &out)
{
    const std::string needle = "\"" + key + "\"";
    const size_t key_pos = data.find(needle);
    if (key_pos == std::string::npos)
        return false;

    const size_t colon_pos = data.find(':', key_pos + needle.size());
    if (colon_pos == std::string::npos)
        return false;

    const size_t open_quote = data.find('"', colon_pos + 1);
    if (open_quote == std::string::npos)
        return false;

    const size_t close_quote = data.find('"', open_quote + 1);
    if (close_quote == std::string::npos)
        return false;

    out = data.substr(open_quote + 1, close_quote - open_quote - 1);
    return true;
}

static void gmcp_parse_supports(const std::string &data,
                                void (*fn)(const std::string &name, int version, void *ctx),
                                void *ctx)
{
    size_t pos = 0;

    while (pos < data.size())
    {
        size_t comma = data.find(',', pos);
        std::string token = (comma == std::string::npos)
            ? data.substr(pos)
            : data.substr(pos, comma - pos);

        token = gmcp_trim_token(token);

        if (!token.empty())
        {
            std::string name;
            int version = 1;

            size_t space = token.find(' ');
            if (space == std::string::npos)
            {
                name = token;
            }
            else
            {
                name = gmcp_trim_token(token.substr(0, space));

                std::string version_str = gmcp_trim_token(token.substr(space + 1));
                if (!version_str.empty())
                    version = atoi(version_str.c_str());
            }

            if (!name.empty())
                fn(name, version, ctx);
        }

        if (comma == std::string::npos)
            break;

        pos = comma + 1;
    }
}

static void gmcp_supports_set_cb(const std::string &name, int version, void *ctx)
{
    if (!ctx)
        return;

    DESCRIPTOR_DATA *d = (DESCRIPTOR_DATA *)ctx;

    if (d->gmcp_cap_count >= MAX_GMCP_CAPS)
        return;

    strncpy(d->gmcp_caps[d->gmcp_cap_count].name, name.c_str(), 63);
    d->gmcp_caps[d->gmcp_cap_count].name[63] = '\0';
    d->gmcp_caps[d->gmcp_cap_count].version = version;
    d->gmcp_cap_count++;
}

static void gmcp_supports_add_cb(const std::string &name, int version, void *ctx)
{
    if (!ctx)
        return;

    DESCRIPTOR_DATA *d = (DESCRIPTOR_DATA *)ctx;
    gmcp_subscribe(d, name);
}

static void gmcp_supports_remove_cb(const std::string &name, int version, void *ctx)
{
    if (!ctx)
        return;

    DESCRIPTOR_DATA *d = (DESCRIPTOR_DATA *)ctx;
    gmcp_unsubscribe(d, name);
}

void gmcp_core_ping(DESCRIPTOR_DATA *d, const std::string &data)
{
    send_gmcp(d, "Core.Ping {}");
}

void gmcp_core_hello(DESCRIPTOR_DATA *d, const std::string &data)
{
    std::string client;
    std::string version;

    if (gmcp_extract_json_string(data, "client", client))
        gmcp_cache_set(d, "Client.Name", client);

    if (gmcp_extract_json_string(data, "version", version))
        gmcp_cache_set(d, "Client.Version", version);
}

void gmcp_sendf(DESCRIPTOR_DATA *d, const std::string &module, const char *fmt, ...)
{
    if (!d || !d->gmcp_enabled || module.empty())
        return;

    if (!gmcp_is_subscribed(d, module))
        return;

    char json[3072];

    if (fmt && fmt[0] != '\0')
    {
        va_list args;
        va_start(args, fmt);
        vsnprintf(json, sizeof(json), fmt, args);
        va_end(args);

        send_gmcp(d, module + " " + json);
    }
    else
    {
        send_gmcp(d, module + " {}");
    }
}

void gmcp_send_all(void (*func)(DESCRIPTOR_DATA *))
{
    DESCRIPTOR_DATA *d;

    for (d = first_descriptor; d; d = d->next)
    {
        if (d->connected == CON_PLAYING && d->gmcp_enabled)
            func(d);
    }
}

void gmcp_core_supports_set(DESCRIPTOR_DATA *d, const std::string &data)
{
    fprintf(stderr, "GMCP: Supports.Set %s\r\n", data.c_str());

    d->gmcp_cap_count = 0;
    gmcp_parse_supports(data, gmcp_supports_set_cb, d);
}

void gmcp_force_resync(CHAR_DATA *ch)
{
    gmcp_evt_char_status(ch);
    gmcp_evt_char_vitals(ch);
    gmcp_evt_char_stats(ch);
    gmcp_evt_room_change(ch);

    if (gmcp_has(ch->desc, "Char"))
    {
        gmcp_send_from_cache(ch->desc, "Char.Status", build_char_status);
        gmcp_send_from_cache(ch->desc, "Char.Vitals", build_char_vitals);
        gmcp_send_from_cache(ch->desc, "Char.Stats", build_char_stats);
    }

    if (true)
        gmcp_send_from_cache(ch->desc, "Room.Info", build_room_info);
}

void gmcp_evt_char_stats(CHAR_DATA *ch)
{
    if (!ch || !ch->desc)
        return;

    DESCRIPTOR_DATA *d = ch->desc;

    bool changed = false;

    changed |= gmcp_cache_set_int(d, "Char.Stats.str", get_curr_str(ch));
    changed |= gmcp_cache_set_int(d, "Char.Stats.dex", get_curr_dex(ch));
    changed |= gmcp_cache_set_int(d, "Char.Stats.con", get_curr_con(ch));
    changed |= gmcp_cache_set_int(d, "Char.Stats.int", get_curr_int(ch));
    changed |= gmcp_cache_set_int(d, "Char.Stats.wis", get_curr_wis(ch));
    changed |= gmcp_cache_set_int(d, "Char.Stats.cha", get_curr_cha(ch));
    changed |= gmcp_cache_set_int(d, "Char.Stats.hitroll", GET_HITROLL(ch));
    changed |= gmcp_cache_set_int(d, "Char.Stats.damroll", GET_DAMROLL(ch));
    changed |= gmcp_cache_set_int(d, "Char.Stats.armor", GET_AC(ch));
    changed |= gmcp_cache_set_int(d, "Char.Stats.align", ch->alignment);
    changed |= gmcp_cache_set_int(d, "Char.Stats.weight", ch->carry_weight);
    changed |= gmcp_cache_set_int(d, "Char.Stats.maxweight", can_carry_w(ch));
    changed |= gmcp_cache_set_int(d, "Char.Stats.numitems", ch->carry_number);
    changed |= gmcp_cache_set_int(d, "Char.Stats.maxnumitems", can_carry_n(ch));

    if (!changed)
        return;

    d->gmcp_pending |= GMCP_PEND_STATS;
}

void gmcp_evt_char_status(CHAR_DATA *ch)
{
    if (!ch || !ch->desc)
        return;

    DESCRIPTOR_DATA *d = ch->desc;

    bool changed = false;

    changed |= gmcp_cache_set(d, "Char.Status.name", ch->name);
    changed |= gmcp_cache_set(d, "Char.Status.fullname", ch->pcdata->title);
    changed |= gmcp_cache_set(d, "Char.Status.class", ability_name[ch->main_ability]);
    changed |= gmcp_cache_set(d, "Char.Status.race", capitalize(get_race(ch)));
    changed |= gmcp_cache_set_int(d, "Char.Status.wimpy", ch->wimpy);
    changed |= gmcp_cache_set(d, "Char.Status.wanted", bitset_to_string(ch->pcdata->wanted_flags, planet_flags));

    changed |= gmcp_cache_set(d, "Char.Status.clan", (ch->pcdata->clan ? ch->pcdata->clan->name : "None"));
    changed |= gmcp_cache_set_int(d, "Char.Status.salary", (ch->pcdata->clan ? ch->pcdata->salary : 0));
    changed |= gmcp_cache_set_int(d, "Char.Status.pkills", (ch->pcdata->clan ? ch->pcdata->pkills : 0));
    changed |= gmcp_cache_set_int(d, "Char.Status.pdeaths", (ch->pcdata->clan ? ch->pcdata->pdeaths : 0));

    if (IS_IMMORTAL(ch))
    {
        changed |= gmcp_cache_set_int(d, "Char.Status.trust", get_trust(ch));
        changed |= gmcp_cache_set_int(d, "Char.Status.wizinvis", (BV_IS_SET(ch->act, PLR_WIZINVIS) ? 0 : ch->pcdata->wizinvis));
        changed |= gmcp_cache_set(d, "Char.Status.Bamfin", (ch->pcdata->bamfin[0] != '\0') ? ch->pcdata->bamfin : "%s appears in a swirling mist.");
        changed |= gmcp_cache_set(d, "Char.Status.Bamfout", (ch->pcdata->bamfout[0] != '\0') ? ch->pcdata->bamfout : "%s appears in a swirling mist.");
        if (ch->pcdata->area)
        {
            changed |= gmcp_cache_set(d, "Char.Status.AreaLoaded", (IS_SET (ch->pcdata->area->status, AREA_LOADED)) ? "yes" : "no");
            changed |= gmcp_cache_set_int(d, "Char.Status.LRVnum", ch->pcdata->area->low_r_vnum);
            changed |= gmcp_cache_set_int(d, "Char.Status.HRVnum", ch->pcdata->area->hi_r_vnum);            
            changed |= gmcp_cache_set_int(d, "Char.Status.LOVnum", ch->pcdata->area->low_o_vnum);            
            changed |= gmcp_cache_set_int(d, "Char.Status.HOVnum", ch->pcdata->area->hi_o_vnum);            
            changed |= gmcp_cache_set_int(d, "Char.Status.LMVnum", ch->pcdata->area->low_m_vnum);            
            changed |= gmcp_cache_set_int(d, "Char.Status.HMVnum", ch->pcdata->area->hi_m_vnum);

        }
    }    

    if (!changed)
        return;
    d->gmcp_pending |= GMCP_PEND_STATUS;
}

void gmcp_evt_char_vitals(CHAR_DATA *ch)
{
    if (!ch || !ch->desc)
        return;

    DESCRIPTOR_DATA *d = ch->desc;

    bool changed = false;

    changed |= gmcp_cache_set_int(d, "Char.Vitals.hp", ch->hit);
    changed |= gmcp_cache_set_int(d, "Char.Vitals.maxhp", ch->max_hit);
    changed |= gmcp_cache_set_int(d, "Char.Vitals.mana", ch->mana);
    changed |= gmcp_cache_set_int(d, "Char.Vitals.maxmana", ch->max_mana);
    changed |= gmcp_cache_set_int(d, "Char.Vitals.move", ch->move);
    changed |= gmcp_cache_set_int(d, "Char.Vitals.maxmove", ch->max_move);
    changed |= gmcp_cache_set_int(d, "Char.Vitals.credits", ch->gold);
    changed |= gmcp_cache_set_int(d, "Char.Vitals.bank", ch->pcdata->bank);
    changed |= gmcp_cache_set_int(d, "Char.Vitals.pkills", ch->pcdata->pkills);
    changed |= gmcp_cache_set_int(d, "Char.Vitals.mkills", ch->pcdata->mkills);

    if (!changed)
        return;
    d->gmcp_pending |= GMCP_PEND_VITALS;
    //gmcp_send_from_cache(d, "Char.Vitals", build_char_vitals);
}

void gmcp_evt_room_change(CHAR_DATA *ch)
{
    if (!ch || !ch->desc || !ch->in_room)
        return;

    DESCRIPTOR_DATA *d = ch->desc;
    ROOM_INDEX_DATA *room = ch->in_room;

    bool changed = false;

    changed |= gmcp_cache_set(d, "Room.Info.num", std::to_string(room->vnum));
    changed |= gmcp_cache_set(d, "Room.Info.name", room->name);
    changed |= gmcp_cache_set_int(d, "Room.Info.rentship",
        (BV_IS_SET(room->room_flags, ROOM_CAN_LAND) ? 1 : 0));
    changed |= gmcp_cache_set_int(d, "Room.Info.serinstop",
        (is_bus_stop(ch->in_room->vnum) ? 1 : 0));

    if (!changed)
        return;

    d->gmcp_pending |= GMCP_PEND_ROOM;
    //gmcp_send_from_cache(d, "Room.Info", build_room_info);
}

/* Check if subscribed */
bool gmcp_is_subscribed(DESCRIPTOR_DATA *d, const std::string &module)
{
    for (int i = 0; i < d->gmcp_sub_count; i++)
    {
        const std::string sub = d->gmcp_subs[i].name;
        if (module.compare(0, sub.size(), sub) == 0)
            return true;
    }
    return false;
}

/* Add subscription */
void gmcp_subscribe(DESCRIPTOR_DATA *d, const std::string &module)
{
    if (d->gmcp_sub_count >= MAX_GMCP_SUBS)
        return;

    if (gmcp_is_subscribed(d, module))
        return;

    strncpy(d->gmcp_subs[d->gmcp_sub_count].name, module.c_str(), 63);
    d->gmcp_subs[d->gmcp_sub_count].name[63] = '\0';
    d->gmcp_sub_count++;
}

/* Remove subscription */
void gmcp_unsubscribe(DESCRIPTOR_DATA *d, const std::string &module)
{
    for (int i = 0; i < d->gmcp_sub_count; i++)
    {
        if (module == d->gmcp_subs[i].name)
        {
            d->gmcp_subs[i] = d->gmcp_subs[d->gmcp_sub_count - 1];
            d->gmcp_sub_count--;
            return;
        }
    }
}

void gmcp_core_supports_add(DESCRIPTOR_DATA *d, const std::string &data)
{
    gmcp_parse_supports(data, gmcp_supports_add_cb, d);
}

void gmcp_core_supports_remove(DESCRIPTOR_DATA *d, const std::string &data)
{
    gmcp_parse_supports(data, gmcp_supports_remove_cb, d);
}

void gmcp_init_subscriptions(DESCRIPTOR_DATA *d)
{
    d->gmcp_sub_count = 0;

    gmcp_subscribe(d, "Char.Vitals");
    gmcp_subscribe(d, "Char.Status");
    gmcp_subscribe(d, "Room.Info");
}

void gmcp_flush(DESCRIPTOR_DATA *d)
{
    if (!d || d->gmcp_pending == 0)
        return;

    /* IMPORTANT:
     * We do NOT combine JSON.
     * We simply send multiple GMCP messages in one burst.
     */

    if (d->gmcp_pending & GMCP_PEND_VITALS && gmcp_has(d, "Char"))
        gmcp_send_from_cache(d, "Char.Vitals", build_char_vitals);

    if (d->gmcp_pending & GMCP_PEND_ROOM)
        gmcp_send_from_cache(d, "Room.Info", build_room_info);

    if (d->gmcp_pending & GMCP_PEND_STATUS && gmcp_has(d, "Char"))
        gmcp_send_from_cache(d, "Char.Status", build_char_status);

    if (d->gmcp_pending & GMCP_PEND_STATS && gmcp_has(d, "Char"))
        gmcp_send_from_cache(d, "Char.Stats", build_char_stats);



    /* clear after sending */
    d->gmcp_pending = 0;
}