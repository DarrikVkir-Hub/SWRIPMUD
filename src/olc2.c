/***************************************************************************
*                                 SWRIP                                    *
*--------------------------------------------------------------------------*
* SWRIP Code Additions and changes from the SWR Code                       *
* copyright (c) 2004+ by Mark Miller / Darrik Vequir                       *
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
* 			New OLC Schema-based System				                       *
* ------------------------------------------------------------------------ *
* 	There are only two hooks currently for this system:                    *
* One in handler.c in char_to_room to handle rwalk                         *
* One in interpret() (two small blocks of code) to intercept OLC commands  *
* Not counting the do_declarations, obviously                              *
****************************************************************************/

    // --------------INDEX----------------
    // In olc.h
    // GENERIC template functions for field handling
    // GENERIC Template Session Control OLC_START OLC_STOP OLC_SET
    // OLC_SHOW templates - handlers for specific field types
    // OLC_SHOW main templates

    // In olc.c
    // OLC defines, enums, and structs - moved to olc.h
    // GENERIC - generic format, string, flag handling that is not OLC specific
    // GENERICOLCSTRING generic format functions but dealing with OLC variables
    // OLC_FORMAT olc_format olc specific format handling
    // OLC_GENERIC olc_generic functions
    // OLC_SHOW olc_show generic functions
    // Session Control OLC_START OLC_STOP OLC_SET
    // DO_COMMANDS DO_OLCSET DO_?EDIT DO_OLC

    // In olc2.c
    // OLCSCHEMA Olc Schema declarations
    // ROOM_OLC room_olc OlcOps functions
    // OLC_ROOM olc_room specific functions
    // OLC_SHOW_ROOM room specific show functions
    // OBJECT_OLC object_olc OlcOps functions
    // OLC_OBJECT olc_object specific functions
    // OLC_SHOW_OBJECT object specific show functions
    // MOBILE_OLC mobile_olc OlcOps functions
    // OLC_MOBILE olc_mobile specific functions
    // OLC_SHOW_MOBILE mobile specific show functions
    // SHOP_OLC shop_olc OlcOps functions
    // OLC_SHOP olc_shop specific functions    
    // DO_COMMANDS DO_?EDIT

#include <typeinfo>
#include <cstring>
#include <set>
#include "mud.h"
#include "olc.h"
#include "craft.h"

// extern functions
extern size_t visible_length(const char *txt);
extern char *wrap_text_ex(const char *txt, int width, int flags, int indent);
extern int get_exflag( const std::string& flag );
extern int get_risflag( const std::string& flag );
extern int get_npc_race( const std::string& type );
extern size_t get_langflag(const std::string& flag);
extern bool command_is_authorized_for_char(CHAR_DATA* ch, CMDTYPE* cmd);
std::vector<CraftRecipe> &craft_get_recipes_for_olc();
CraftRecipe *craft_find_recipe_for_olc( const std::string &name );
void craft_save_all_recipes_for_olc();


bool olc_extradescs_equal(const EXTRA_DESCR_DATA* a, const EXTRA_DESCR_DATA* b);
bool olc_affects_equal(const AFFECT_DATA* a, const AFFECT_DATA* b);
void olc_apply_flag_delta(BitSet& dst, const BitSet& edited, const BitSet& baseline);
MatchResult match_keywords(const std::string& keyword_list, const std::string& input);
void olc_show_extradesc_help(CHAR_DATA* ch);
bool olc_format_parse_strict_int(const std::string& s, int& out);
std::string olc_format_trim_left(std::string s);
EXIT_DATA* find_exit_by_number(ROOM_INDEX_DATA* room, int index);
EXIT_DATA* find_exit_by_dir_index(ROOM_INDEX_DATA* room, int dir, int index);
EXIT_DATA* find_exit_by_keyword(ROOM_INDEX_DATA* room, const std::string& keyword);
const char* exit_dir_label(int dir);
int get_rev_dir(int dir);
std::string  olc_format_exit_flags_list();

//internal functions
std::string olc_object_values_summary(const OBJ_DATA* obj);
std::string olc_object_proto_values_summary(const OBJ_INDEX_DATA* obj);
bool olc_object_values_equal_display(const OBJ_DATA* obj, const OBJ_INDEX_DATA* proto);
bool olc_object_in_edit_mode(CHAR_DATA* ch);
void olc_show_object_extradesc_help(CHAR_DATA* ch);
AFFECT_DATA* olc_find_object_affect_by_number(OBJ_DATA* obj, int index);
bool olc_object_parse_affect_value(CHAR_DATA* ch, int loc, const std::string& value_text, int& out_value);
bool olc_object_finish_extradesc_edit( CHAR_DATA* ch, OBJ_DATA* obj, const std::string& value);

void olc_mobile_reset_pending_proto(OlcSession* sess);
void olc_mobile_init_pending_proto_from_live(OlcSession* sess, CHAR_DATA* mob);
bool olc_mobile_set_race(CHAR_DATA* mob, const std::string& value);
bool olc_mobile_set_level(CHAR_DATA* mob, const std::string& value);
bool olc_mobile_set_speaking(CHAR_DATA* mob, const std::string& value);
bool olc_mobile_set_ris_field(CHAR_DATA* mob, int CHAR_DATA::*member, const std::string& value);
std::string olc_mobile_get_ris_field(CHAR_DATA* mob, int CHAR_DATA::*member);
bool olc_mobile_set_legacy_bits_field(CHAR_DATA* mob, int CHAR_DATA::*member, const std::string& value, const char* const* table);
std::string olc_mobile_get_legacy_bits_field(CHAR_DATA* mob, int CHAR_DATA::*member, const char* const* table);
bool olc_mobile_set_spec(CHAR_DATA* mob, const std::string& value);
bool olc_mobile_set_spec2(CHAR_DATA* mob, const std::string& value);
bool olc_mobile_set_long_field(CHAR_DATA* mob, const std::string& value);
std::string olc_mobile_get_pending_proto_die_field(CHAR_DATA* ch, CHAR_DATA* mob, const std::string& field);
bool olc_mobile_set_pending_proto_die_field(CHAR_DATA* ch, const std::string& field, const std::string& value);
bool olc_mobile_in_edit_mode(CHAR_DATA* ch);
AFFECT_DATA* olc_mobile_find_affect_by_number(CHAR_DATA* mob, int index);
bool olc_mobile_parse_affect_value( CHAR_DATA* ch, int loc, const std::string& value_text, int& out_value);


void olc_room_edit_help(CHAR_DATA* ch);
void olc_room_relink_exits(ROOM_INDEX_DATA* room);
void olc_room_apply_pending_exit_side_effect(const OlcPendingExitSideEffect& p);
void olc_show_room_exit_help(CHAR_DATA* ch);
bool olc_room_finish_extradesc_edit( CHAR_DATA* ch, ROOM_INDEX_DATA* room, const std::string& value );

SHOP_DATA* olc_shop_clone(const SHOP_DATA* src);
void olc_shop_free(SHOP_DATA* shop);
void olc_shop_apply_instance_changes(SHOP_DATA* dst, const SHOP_DATA* src);
void olc_show_shop(CHAR_DATA* ch, SHOP_DATA* shop, bool show_help, int term_width);
bool olc_shop_in_edit_mode(CHAR_DATA* ch);

static const char *craft_consume_mode_name( CraftConsumeMode mode );
static bool craft_consume_mode_from_string( const std::string &value, CraftConsumeMode &out );
static bool craft_item_type_from_string( const std::string &value, int &out );
static std::string craft_slot_summary( const CraftRecipe *recipe );
static void olc_craft_show_slot_help( CHAR_DATA *ch );
static int olc_craft_find_slot_index( const CraftRecipe *recipe, const std::string &which );
static bool olc_craft_edit_slots_field( CHAR_DATA *ch, CraftRecipe *recipe );
static bool craft_is_valid_derived_source_field( const std::string &field );
static bool craft_is_valid_derived_aggregate_op( const std::string &op );
static std::string craft_derived_var_summary( const CraftRecipe *recipe );
static void olc_craft_show_derived_var_help( CHAR_DATA *ch );
static int olc_craft_find_derived_var_index( const CraftRecipe *recipe, const std::string &which );
static bool olc_craft_edit_derived_vars_field( CHAR_DATA *ch, CraftRecipe *recipe );
static bool craft_is_valid_assignment_target( const std::string &target );
static std::string craft_assignment_summary( const CraftRecipe *recipe );
static void olc_craft_show_assignment_help( CHAR_DATA *ch );
static int olc_craft_find_assignment_index( const CraftRecipe *recipe, const std::string &which );
static bool olc_craft_edit_assignments_field( CHAR_DATA *ch, CraftRecipe *recipe );
static std::string craft_affect_bitvector_name( int bitvector );
static std::string craft_affect_summary( const CraftRecipe *recipe );
static void olc_craft_show_affect_help( CHAR_DATA *ch );
static int olc_craft_find_affect_index( const CraftRecipe *recipe, const std::string &which );
static bool olc_craft_edit_affects_field( CHAR_DATA *ch, CraftRecipe *recipe );
static bool craft_is_valid_result_target( const std::string &target );
static void craft_collect_expr_var_refs( const CraftExpr &expr, std::set<std::string> &out );
static bool craft_validate_expr_vars( const CraftExpr &expr, const std::set<std::string> &valid_vars, std::string &err );
static bool craft_validate_recipe( const CraftRecipe *recipe, std::vector<std::string> &errors );
static bool craft_validate_recipe_report( CHAR_DATA *ch, const CraftRecipe *recipe, const char *label );
static bool olc_craft_is_field_name( const char *input );
static bool olc_craft_edit_interpret( CHAR_DATA *ch, const std::string &command, const std::string &argument );
bool olc_craft_edit_revert(CHAR_DATA* ch);

template <typename T>
OlcField<T> make_olc_std_string_field(
    const char* name,
    std::string T::*member,
    const char* help)
{
    return make_olc_custom_value_field<T>(
        name,
        OlcValueType::STRING,
        nullptr,
        OlcMetaType::NONE,
        [member](T* obj, const std::string& value) -> bool
        {
            obj->*member = value;
            return true;
        },
        [member](T* obj) -> std::string
        {
            return obj->*member;
        },
        help
    );
}

static std::string craft_flag_vector_to_string(
    const std::vector<int> &flags,
    const flag_name *table)
{
    std::string out;

    for ( size_t i = 0; i < flags.size(); ++i )
    {
        std::string name = enum_to_string_flag( flags[i], table );

        if ( name.empty() )
            name = std::to_string( flags[i] );

        if ( !out.empty() )
            out += " ";

        out += name;
    }

    return out.empty() ? "none" : out;
}

static bool craft_flag_vector_from_string(
    std::vector<int> &dst,
    const std::string &value,
    const flag_name *table)
{
    std::istringstream iss(value);
    std::string token;
    std::vector<int> out;

    while ( iss >> token )
    {
        int bit = -1;

        if ( !enum_from_string_flag(token, bit, table) )
            return false;

        out.push_back(bit);
    }

    dst = std::move(out);
    return true;
}

// --------------------------------------------
// OLCSCHEMA Olc Schema declarations
// --------------------------------------------

OlcField<OBJ_DATA> make_olc_object_value_field(
    const char* name,
    int index,
    const char* help)
{
    return OlcField<OBJ_DATA>{
        name,
        OlcValueType::INT,
        nullptr,
        OlcMetaType::NONE,
        nullptr,
        [index](OBJ_DATA* obj, const std::string& value) -> bool
        {
            return set_int_field(obj->value[index], value, INT_MIN, INT_MAX);
        },
        [index](OBJ_DATA* obj) -> std::string
        {
            return get_int_field(obj->value[index]);
        },
        nullptr,
        nullptr,
        0,
        help,
        INT_MIN,
        INT_MAX,
        false
    };
}

const OlcSchema<ROOM_INDEX_DATA>* get_room_schema()
{
    static std::vector<OlcField<ROOM_INDEX_DATA>> room_fields =
    {
        make_olc_string_nohash_field<ROOM_INDEX_DATA>("name", &ROOM_INDEX_DATA::name,
            "Room Name - Seen at the top when looking at a room"),
        make_olc_editor_field<ROOM_INDEX_DATA>("description", &ROOM_INDEX_DATA::description,
            SUB_ROOM_DESC, "Room's long description which shows when looking/moving and not +BRIEF"),
        make_olc_custom_editor_field<ROOM_INDEX_DATA>("extradesc", OlcMetaType::EXTRA_DESC_LIST,
            [](CHAR_DATA* ch, ROOM_INDEX_DATA* room) -> bool { return olc_room_edit_extradesc_field(ch, room); },
            [](ROOM_INDEX_DATA* room) -> std::string { return olc_room_extradesc_list_summary(room); },
            [](CHAR_DATA* ch, ROOM_INDEX_DATA* room, const std::string& value) -> bool { return olc_room_finish_extradesc_edit(ch, room, value); },  
            SUB_ROOM_EXTRA,
            "These provide extra 'look (keyword)' that can provide more room depth.\nExample: If you describe a painting in the long description, an extra description can describe what the painting looks like when 'look painting'",
            true),
        make_olc_enum_flag_field<ROOM_INDEX_DATA>("sector_type", &ROOM_INDEX_DATA::sector_type,
            sect_types, "Room Terrain - Determines ambiance messages that get sent regularly"),
        make_olc_flag_field<ROOM_INDEX_DATA>("flags", &ROOM_INDEX_DATA::room_flags,
            r_flags, "Room flags: type help flags for flag meaning"),
        make_olc_int_field<ROOM_INDEX_DATA>("tunnel", &ROOM_INDEX_DATA::tunnel,
            "Max occupants that can be in the room", 0, 200),
        make_olc_int_field<ROOM_INDEX_DATA>("televnum", &ROOM_INDEX_DATA::tele_vnum,
            "Teleportation Destination", INT_MIN, INT_MAX),
        make_olc_int_field<ROOM_INDEX_DATA>("teledelay", &ROOM_INDEX_DATA::tele_delay,
            "Delay before teleportation", INT_MIN, INT_MAX),
        make_olc_custom_editor_field<ROOM_INDEX_DATA>("exit", OlcMetaType::EXIT_LIST,
            [](CHAR_DATA* ch, ROOM_INDEX_DATA* room) -> bool { return olc_room_edit_exit_field(ch, room); },
            [](ROOM_INDEX_DATA* room) -> std::string { return olc_room_exit_list_summary(room); },
            nullptr, 0, "Manage room exits - type exit ? for more info", true),
        make_olc_custom_editor_field<ROOM_INDEX_DATA>("bexit", OlcMetaType::EXIT_LIST,
            [](CHAR_DATA* ch, ROOM_INDEX_DATA* room) -> bool { return olc_room_edit_bexit_field(ch, room); },
            [](ROOM_INDEX_DATA* room) -> std::string { return olc_room_exit_list_summary(room); },
            nullptr, 0, "Manage bidirectional exit changes - type bexit ? for more info", true),
    };

    static OlcSchema<ROOM_INDEX_DATA> room_schema;
    room_schema.name = "room";
    room_schema.fields = &room_fields;
    return &room_schema;
}

const OlcSchema<OBJ_DATA>* get_object_schema()
{
    static std::vector<OlcField<OBJ_DATA>> object_fields =
    {
        make_olc_string_field<OBJ_DATA>("name", &OBJ_DATA::name,
            "Object keywords - this is what is checked with commands like get, wear, etc"),
        make_olc_string_field<OBJ_DATA>("short", &OBJ_DATA::short_descr,
            "Short description - often used within a string, like 'You get (short)' so you should not use capitals or punctuation here"),
        make_olc_editor_field<OBJ_DATA>("description", &OBJ_DATA::description,
            SUB_OBJ_LONG, "What you will see when the object is on the ground"),
        make_olc_string_field<OBJ_DATA>("action", &OBJ_DATA::action_desc,
            "string used when object is used - $n/$N name (person/target) $m/$M him/her/it (person/target) $p short desc.  Won't be used for most objects"),
        make_olc_enum_flag_field<OBJ_DATA>("type", &OBJ_DATA::item_type,
            o_types, "Type of the object"),
        make_olc_flag_field<OBJ_DATA>("wearflags", &OBJ_DATA::wear_flags,
            w_flags, "Locations where this object can be worn.  If you don't have take, it can't be picked up"),
        make_olc_flag_field<OBJ_DATA>("flags", &OBJ_DATA::objflags,
            obj_flag_table, "This includes pipe flags, weapon type, and normal object flags.  Only the first (in flag order) of the weapon types will be considered"),
        make_olc_flag_field<OBJ_DATA>("trigflags", &OBJ_DATA::trig_flags,
            trig_flags, "This includes trigger specific flags, used only for levers, switches, buttons, and pull chains.  HELP TRIGFLAGS for more info"),
        make_olc_int_field<OBJ_DATA>("wearloc", &OBJ_DATA::wear_loc,
            "Current wear location - for fixing bugs, but shouldn't be manually set normally"),
        make_olc_int_field<OBJ_DATA>("weight", &OBJ_DATA::weight,
            "The weight of the object - the higher the weight, the more weight this will take up in a char's inventory"),
        make_olc_int_field<OBJ_DATA>("cost", &OBJ_DATA::cost,
            "The base cost of an object"),
        make_olc_int_field<OBJ_DATA>("level", &OBJ_DATA::level,
            "Level really only determines it's resistance to magic and how much damage an embedded spell does"),
        make_olc_int_field<OBJ_DATA>("timer", &OBJ_DATA::timer,
            "How long an object will last before decaying"),
        make_olc_int_field<OBJ_DATA>("count", &OBJ_DATA::count,
            "This field sets how many of these objects are stacked in this.  It's a real count, so only used when you want more than one of these objects to be available by default."),
        make_olc_object_value_field("value0", 0, "These values has variable meanings - 'show' will display what they are used for according to the object type you set"),
        make_olc_object_value_field("value1", 1, "These values has variable meanings - 'show' will display what they are used for according to the object type you set"),
        make_olc_object_value_field("value2", 2, "These values has variable meanings - 'show' will display what they are used for according to the object type you set"),
        make_olc_object_value_field("value3", 3, "These values has variable meanings - 'show' will display what they are used for according to the object type you set"),
        make_olc_object_value_field("value4", 4, "These values has variable meanings - 'show' will display what they are used for according to the object type you set"),
        make_olc_object_value_field("value5", 5, "These values has variable meanings - 'show' will display what they are used for according to the object type you set"),
        make_olc_custom_editor_field<OBJ_DATA>("extradesc", OlcMetaType::EXTRA_DESC_LIST,
            [](CHAR_DATA* ch, OBJ_DATA* obj) -> bool { return olc_object_edit_extradesc_field(ch, obj); },
            [](OBJ_DATA* obj) -> std::string { return olc_object_extradesc_list_summary(obj); },
            [](CHAR_DATA* ch, OBJ_DATA* obj, const std::string& value) -> bool { return olc_object_finish_extradesc_edit(ch, obj, value); },            
            SUB_OBJ_EXTRA, "These provide extra 'look (keyword)' that can provide more object depth.", true),
        make_olc_custom_editor_field<OBJ_DATA>("affect", OlcMetaType::OBJ_AFFECT_LIST,
            [](CHAR_DATA* ch, OBJ_DATA* obj) -> bool { return olc_edit_object_affect_field(ch, obj); },
            [](OBJ_DATA* obj) -> std::string { return olc_object_affect_list_summary(obj); },
            nullptr, 0, "Object affects", true)
    };

    static OlcSchema<OBJ_DATA> object_schema;
    object_schema.name = "object";
    object_schema.fields = &object_fields;
    return &object_schema;
}


const OlcSchema<CHAR_DATA>* get_mobile_schema()
{
static std::vector<OlcField<CHAR_DATA>> mob_fields =
{
    make_olc_string_field<CHAR_DATA>( "name", &CHAR_DATA::name, "Mob keywords - used for get, wear, etc" ),
    make_olc_string_field<CHAR_DATA>( "short", &CHAR_DATA::short_descr, "Short description used when the mob is described in an action - Should only be capitalized if it's a name - '(short) moves south', 'You attack (short)" ),
    OlcField<CHAR_DATA>{ "long", OlcValueType::STRING, nullptr, OlcMetaType::NONE, nullptr,
        [](CHAR_DATA* mob, const std::string& value) -> bool { return olc_mobile_set_long_field(mob, value); },
        [](CHAR_DATA* mob) -> std::string { return mob->long_descr ? mob->long_descr : ""; },
        nullptr, nullptr, 0, "Long is used when seeing the mob in a room", INT_MIN, INT_MAX, false },
    make_olc_editor_field<CHAR_DATA>( "description", &CHAR_DATA::description, SUB_MOB_DESC, "What shows when you look/examine a char.  When not set, you see 'nothing special', so not required" ),
    make_olc_int_field<CHAR_DATA>( "sex", &CHAR_DATA::sex, "0 for neutral, 1 for male, 2 for female", 0, 2 ),
    OlcField<CHAR_DATA>{ "race", OlcValueType::ENUM, nullptr, OlcMetaType::NONE, nullptr,
        [](CHAR_DATA* mob, const std::string& value) -> bool { return olc_mobile_set_race(mob, value); },
        [](CHAR_DATA* mob) -> std::string {
            int r = mob->race;
            if (r >= 0 && r < MAX_NPC_RACE)
                return get_flag_name(npc_race, r, MAX_NPC_RACE);
            return std::to_string(r);
        },
        nullptr, nullptr, 0, "NPC race - more selections than player races", INT_MIN, INT_MAX, false },
    OlcField<CHAR_DATA>{ "level", OlcValueType::INT, nullptr, OlcMetaType::NONE, nullptr,
        [](CHAR_DATA* mob, const std::string& value) -> bool { return olc_mobile_set_level(mob, value); },
        [](CHAR_DATA* mob) -> std::string { return get_int_field(mob->top_level); },
        nullptr, nullptr, 0,
        "How powerful the mob is - if you sit this, it will override armor/hitroll/damroll to what is appropriate for that mob level.  Also used in 'consider' so be careful not to trick a player by setting the level low and their rolls high.  This also affects how much xp a char gets for killing it",
        0, LEVEL_AVATAR + 5, false },
    make_olc_int_field<CHAR_DATA>( "alignment", &CHAR_DATA::alignment, "How good or evil is the mob?  Between -1000 and 1000", -1000, 1000 ),
    make_olc_int_field<CHAR_DATA>( "armor", &CHAR_DATA::armor, "Bare skin armor value", -300, 300 ),
    make_olc_int_field<CHAR_DATA>( "hitroll", &CHAR_DATA::hitroll, "Hitroll - defaults to level / 5.  Tweak it, don't play with it", 0, 85 ),
    make_olc_int_field<CHAR_DATA>( "damroll", &CHAR_DATA::damroll, "Damroll - defaults to level / 5.  Tweak it, don't play with it", 0, 65 ),
    make_olc_int_field<CHAR_DATA>( "hitplus", &CHAR_DATA::hitplus, "A flat bonus to hitpoints" ),
    make_olc_int_field<CHAR_DATA>( "damplus", &CHAR_DATA::damplus, "A flat bonus to damage" ),
    make_olc_int_field<CHAR_DATA>( "numattacks", &CHAR_DATA::numattacks, "Whether the mob hits once, or more than once, at a time", 0, 20 ),
    make_olc_int_field<CHAR_DATA>( "position", &CHAR_DATA::position,
        "Current position - 0 through 11, in this order - dead, mortal, incapacitated, stunned, sleeping, resting, sitting, fighting, standing, mounted, shove, drag",
         0, POS_STANDING ),
    make_olc_int_field<CHAR_DATA>( "defposition", &CHAR_DATA::defposition,
        "Default position - 0 through 11, in this order - dead, mortal, incapacitated, stunned, sleeping, resting, sitting, fighting, standing, mounted, shove, drag",
        0, POS_STANDING ),
    make_olc_int_field<CHAR_DATA>( "height", &CHAR_DATA::height, "Height of the mob" ),
    make_olc_int_field<CHAR_DATA>( "weight", &CHAR_DATA::weight, "Weight of the mob" ),
    make_olc_int_field<CHAR_DATA>( "credits", &CHAR_DATA::gold, "Credits/Money the mob starts with" ),
    make_olc_int_field<CHAR_DATA>( "str", &CHAR_DATA::perm_str, "Generally, strength determines how much weight can be carried and how strong with physical skills", 1, 25 ),
    make_olc_int_field<CHAR_DATA>( "int", &CHAR_DATA::perm_int, "Generally, intelligence determines how quickly you can learn, how fast you recover force, and shares with wis success chances for force", 1, 25 ),
    make_olc_int_field<CHAR_DATA>( "wis", &CHAR_DATA::perm_wis, "Generally, wisdom determines success chance of force and learning abilities", 1, 25 ),
    make_olc_int_field<CHAR_DATA>( "dex", &CHAR_DATA::perm_dex, "Generally, dexterity determines how many items can be carried and how successful with physical skills", 1, 25 ),
    make_olc_int_field<CHAR_DATA>( "con", &CHAR_DATA::perm_con, "Generally, constitution determines recovery time for mental state and hit points", 1, 25 ),
    make_olc_int_field<CHAR_DATA>( "cha", &CHAR_DATA::perm_cha, "Generally, charisma determines success rates with skills like propeganda and seduce, as well as shop prices", 1, 25 ),
    make_olc_int_field<CHAR_DATA>( "lck", &CHAR_DATA::perm_lck, "Generally, luck modifies your success chances in a lot of various things", 1, 25 ),
    make_olc_int_field<CHAR_DATA>( "frc", &CHAR_DATA::perm_frc, "Force affinity determines how strong your connection to the force is", 0, 20 ),
    make_olc_int_field<CHAR_DATA>( "sav1", &CHAR_DATA::saving_poison_death, "+/-perc Save vs poison - used in poisoned weapon checks and poison spells", -30, 30 ),
    make_olc_int_field<CHAR_DATA>( "sav2", &CHAR_DATA::saving_wand, "+/-perc Save vs wand - Very rarely used", -30, 30 ),
    make_olc_int_field<CHAR_DATA>( "sav3", &CHAR_DATA::saving_para_petri, "+/-perc Save vs paralysis/petrification - used to check for stun and paralysis in fights and spells", -30, 30 ),
    make_olc_int_field<CHAR_DATA>( "sav4", &CHAR_DATA::saving_breath, "+/-perc Save vs breath - only used on breath attacks, so not used in SWRIP", -30, 30 ),
    make_olc_int_field<CHAR_DATA>( "sav5", &CHAR_DATA::saving_spell_staff, "+/-perc Save vs spell/staff - generic save against most spells, so I'd call it general force resistance", -30, 30 ),
    make_olc_flag_field<CHAR_DATA>( "flags", &CHAR_DATA::act, act_flags, "NPC specific flags" ),
    make_olc_flag_field<CHAR_DATA>( "affected", &CHAR_DATA::affected_by, aff_flags, "What the mob is affected by by default." ),
    make_olc_flag_field<CHAR_DATA>( "vip", &CHAR_DATA::vip_flags, planet_flags, "What planet the mob is a citizen of - determines if there's a wanted flag applied on being killed" ),
    make_olc_int_field<CHAR_DATA>( "force", &CHAR_DATA::max_mana, "Force points - overall mana (yes, I said it) pool" ),
    make_olc_int_field<CHAR_DATA>( "move", &CHAR_DATA::max_move, "Movement points - basically?  Stamina - you need it to move and do some other physical skills" ),
    make_olc_int_field<CHAR_DATA>( "hp", &CHAR_DATA::max_hit, "Max hit points" ),
    make_olc_int_field<CHAR_DATA>( "carry_weight", &CHAR_DATA::carry_weight, "How much they are carrying - you shouldn't need to set this unless something is broken, as this does not determine their maximum carry, that's strength" ),
    make_olc_flag_field<CHAR_DATA>( "speaks", &CHAR_DATA::speaks, lang_names, "What languages the mob can understand" ),
    make_olc_flag_field<CHAR_DATA>( "resistant", &CHAR_DATA::resistant, ris_flags, 
        "Resistances (RIS flags) - Reduces the damage taken corresponding to the damage type" ),
    make_olc_flag_field<CHAR_DATA>( "immune", &CHAR_DATA::immune, ris_flags, 
        "Immunities (RIS flags) - Makes the mob immune to damage taken corresponding to the damage type" ),
    make_olc_flag_field<CHAR_DATA>( "susceptible", &CHAR_DATA::susceptible, ris_flags, 
        "Susceptibilities (RIS flags) - Increases the damage taken corresponding to the damage type" ),

        OlcField<CHAR_DATA>{ "speaking", OlcValueType::ENUM, (const void*)lang_names, OlcMetaType::FLAG_TABLE, nullptr,
        [](CHAR_DATA* mob, const std::string& value) -> bool { return olc_mobile_set_speaking(mob, value); },
        [](CHAR_DATA* mob) -> std::string {
            if (mob->speaking >= 0 && mob->speaking < LANG_MAX)
                return lang_names[mob->speaking].name;
            return "unknown"; },
        nullptr, nullptr, 0, "Current spoken language, help speaks to see the list", INT_MIN, INT_MAX, false },
    OlcField<CHAR_DATA>{ "parts", OlcValueType::FLAG, (const void*)part_flags, OlcMetaType::ENUM_LEGACY, nullptr, nullptr,
        [](CHAR_DATA* mob) -> std::string { return olc_mobile_get_legacy_bits_field(mob, &CHAR_DATA::xflags, part_flags); },
        [](CHAR_DATA* /*ch*/, CHAR_DATA* mob, const std::string& value) -> bool { return olc_mobile_set_legacy_bits_field(mob, &CHAR_DATA::xflags, value, part_flags); },
        nullptr, 0, "Body parts the mob has", INT_MIN, INT_MAX, false },
    OlcField<CHAR_DATA>{ "attack", OlcValueType::FLAG, (const void*)attack_flags, OlcMetaType::ENUM_LEGACY, nullptr, nullptr,
        [](CHAR_DATA* mob) -> std::string { return olc_mobile_get_legacy_bits_field(mob, &CHAR_DATA::attacks, attack_flags); },
        [](CHAR_DATA* /*ch*/, CHAR_DATA* mob, const std::string& value) -> bool { return olc_mobile_set_legacy_bits_field(mob, &CHAR_DATA::attacks, value, attack_flags); },
        nullptr, 0, "What types of attacks the mob will try to use beyond the default", INT_MIN, INT_MAX, false },
    OlcField<CHAR_DATA>{ "defense", OlcValueType::FLAG, (const void*)defense_flags, OlcMetaType::ENUM_LEGACY, nullptr, nullptr,
        [](CHAR_DATA* mob) -> std::string { return olc_mobile_get_legacy_bits_field(mob, &CHAR_DATA::defenses, defense_flags); },
        [](CHAR_DATA* /*ch*/, CHAR_DATA* mob, const std::string& value) -> bool { return olc_mobile_set_legacy_bits_field(mob, &CHAR_DATA::defenses, value, defense_flags); },
        nullptr, 0, "Gives immunity to the type of attacks these defend against", INT_MIN, INT_MAX, false },
    OlcField<CHAR_DATA>{ "spec", OlcValueType::STRING, nullptr, OlcMetaType::NONE, nullptr, nullptr,
        [](CHAR_DATA* mob) -> std::string { return mob->spec_fun ? lookup_spec( mob->spec_fun ) : "none"; },
        [](CHAR_DATA* /*ch*/, CHAR_DATA* mob, const std::string& value) -> bool { return olc_mobile_set_spec(mob, value); },
        nullptr, 0, "Primary special function name, or 'none' - these are special functions like police, thief, janitor, etc.  SEE HELP SPEC for list", INT_MIN, INT_MAX, false },
    OlcField<CHAR_DATA>{ "spec2", OlcValueType::STRING, nullptr, OlcMetaType::NONE, nullptr, nullptr,
        [](CHAR_DATA* mob) -> std::string { return mob->spec_2 ? lookup_spec( mob->spec_2 ) : "none"; },
        [](CHAR_DATA* /*ch*/, CHAR_DATA* mob, const std::string& value) -> bool { return olc_mobile_set_spec2(mob, value); },
        nullptr, 0, "Secondary special function name, or 'none' - these are special functions like police, thief, janitor, etc.  SEE HELP SPEC for list", INT_MIN, INT_MAX, false },
    OlcField<CHAR_DATA>{ "hitnodice", OlcValueType::INT, nullptr, OlcMetaType::NONE, nullptr, nullptr, nullptr,
        [](CHAR_DATA* ch, void* /*obj*/, const std::string& value) -> bool { return olc_mobile_set_pending_proto_die_field(ch, "hitnodice", value); },
        [](CHAR_DATA* ch, void* obj) -> std::string { return olc_mobile_get_pending_proto_die_field( ch, static_cast<CHAR_DATA*>(obj), "hitnodice"); },
        0, "Prototype hit dice count - Used to calculate the mob's hitpoints along with hitsizedice and hitplus", 0, 32767, false },
    OlcField<CHAR_DATA>{ "hitsizedice", OlcValueType::INT, nullptr, OlcMetaType::NONE, nullptr, nullptr, nullptr,
        [](CHAR_DATA* ch, void* /*obj*/, const std::string& value) -> bool { return olc_mobile_set_pending_proto_die_field(ch, "hitsizedice", value); },
        [](CHAR_DATA* ch, void* obj) -> std::string { return olc_mobile_get_pending_proto_die_field( ch, static_cast<CHAR_DATA*>(obj), "hitsizedice"); },
        0, "Prototype hit dice size - Used to calculate the mob's hitpoints along with hitnodice and hitplus", 0, 32767, false },
    OlcField<CHAR_DATA>{ "damnodice", OlcValueType::INT, nullptr, OlcMetaType::NONE, nullptr, nullptr, nullptr,
        [](CHAR_DATA* ch, void* /*obj*/, const std::string& value) -> bool { return olc_mobile_set_pending_proto_die_field(ch, "damnodice", value); },
        [](CHAR_DATA* ch, void* obj) -> std::string { return olc_mobile_get_pending_proto_die_field( ch, static_cast<CHAR_DATA*>(obj), "damnodice"); },
        0, "Prototype damage dice count - Used to calculate the mob's damage along with damsizedice and damplus", 0, 32767, false  },
    OlcField<CHAR_DATA>{ "damsizedice", OlcValueType::INT, nullptr, OlcMetaType::NONE, nullptr, nullptr, nullptr,
        [](CHAR_DATA* ch, void* /*obj*/, const std::string& value) -> bool { return olc_mobile_set_pending_proto_die_field(ch, "damsizedice", value); },
        [](CHAR_DATA* ch, void* obj) -> std::string { return olc_mobile_get_pending_proto_die_field( ch, static_cast<CHAR_DATA*>(obj), "damsizedice"); },
        0, "Prototype damage dice size - Used to calculate the mob's damage along with damnodice and damplus", 0, 32767, false },
    make_olc_custom_editor_field<CHAR_DATA>( "affect", OlcMetaType::MOB_AFFECT_LIST,
        [](CHAR_DATA* ch, CHAR_DATA* mob) -> bool { return olc_mobile_edit_affect_field(ch, mob); },
        [](CHAR_DATA* mob) -> std::string { return olc_mobile_affect_list_summary(mob); },
        nullptr, 0, "Mobile affects", true ),
};

    static OlcSchema<CHAR_DATA> mob_schema;
    mob_schema.name = "mobile";
    mob_schema.fields = &mob_fields;
    return &mob_schema;
}

const OlcSchema<SHOP_DATA>* get_shop_schema()
{
    static std::vector<OlcField<SHOP_DATA>> shop_fields =
    {
        //make_olc_int_field<SHOP_DATA>("keeper", &SHOP_DATA::keeper,   // Not supporting changing the keeper vnum yet, although 
        //    "Vnum of shop keeper mob", INT_MIN, INT_MAX),             // it'd be a good thing to add in the future - DV 4-8-26
        make_olc_int_field<SHOP_DATA>("open", &SHOP_DATA::open_hour,
            "Opening Hour - Time the shop opens", 0, 23),
        make_olc_int_field<SHOP_DATA>("close", &SHOP_DATA::close_hour,
            "Closing Hour - Time the shop closes", 0, 23),
        make_olc_int_field<SHOP_DATA>("buy", &SHOP_DATA::profit_buy,
            "Profit Margin (Buy) - Percentage the shop charges for items it buys", 100, 200),
        make_olc_int_field<SHOP_DATA>("sell", &SHOP_DATA::profit_sell,
            "Profit Margin (Sell) - Percentage the shop charges for items it sells", 0, 100),
        make_olc_int_field<SHOP_DATA>("fix", &SHOP_DATA::profit_fix,
            "Profit Margin (Fix) - Percentage the shop charges for fixing items", 0, 100),            
        make_olc_flag_field<SHOP_DATA>("stype", &SHOP_DATA::shop_type,
            shop_types, "Shop types: Determines what the shop will do"),            
        make_olc_flag_field<SHOP_DATA>("itypes", &SHOP_DATA::buy_type,
            o_types, "Item types: the object types the shop will handle"),              
    };

    static OlcSchema<SHOP_DATA> shop_schema;
    shop_schema.name = "shop";
    shop_schema.fields = &shop_fields;
    return &shop_schema;
}

static const char *craft_room_req_names[] =
{
    "none",
    "factory",
    nullptr
};

static const char *craft_output_mode_names[] =
{
    "create_proto",
    "transform_base",
    nullptr
};

const OlcSchema<CraftRecipe>* get_craft_schema()
{
    static std::vector<OlcField<CraftRecipe>> craft_fields =
    {
        make_olc_std_string_field<CraftRecipe>("name", &CraftRecipe::name, "Internal recipe name used by select/cfedit/save"),
        make_olc_std_string_field<CraftRecipe>( "display", &CraftRecipe::display_name, "Displayed recipe name"),
        make_olc_custom_value_field<CraftRecipe>( "gsn", OlcValueType::STRING, nullptr, OlcMetaType::NONE,
            [](CraftRecipe* r, const std::string& value) -> bool
            {
                std::string s = value;
                if ( s.empty() || !str_cmp(s, "none") )
                {
                    r->gsn = -1;
                    return true;
                }

                int gsn = skill_lookup(s);
                if ( gsn >= 0 )
                {
                    r->gsn = gsn;
                    return true;
                }

                try
                {
                    r->gsn = std::stoi(s);
                    return true;
                }
                catch (...)
                {
                    return false;
                }
            },
            [](CraftRecipe* r) -> std::string
            {
                if ( r->gsn < 0 || !skill_table[r->gsn] )
                    return "none";
                return skill_table[r->gsn]->name;
            },
            "Required skill name, skill number, or 'none'"
        ),

        make_olc_custom_value_field<CraftRecipe>( "roomreq", OlcValueType::ENUM, (const void*)craft_room_req_names, OlcMetaType::ENUM_LEGACY,
            [](CraftRecipe* r, const std::string& value) -> bool
            {
                int v;
                if ( !enum_from_string_legacy(value, v, craft_room_req_names) )
                    return false;

                r->room_req = static_cast<CraftRoomRequirement>(v);
                return true;
            },
            [](CraftRecipe* r) -> std::string
            {
                return enum_to_string_legacy((int)r->room_req, craft_room_req_names);
            },
            "Room requirement: none or factory"
        ),

        make_olc_int_field<CraftRecipe>( "timer", &CraftRecipe::timer_ticks, "Craft timer in pulses/ticks", 0, INT_MAX),

        make_olc_custom_value_field<CraftRecipe>( "output", OlcValueType::ENUM, (const void*)craft_output_mode_names, OlcMetaType::ENUM_LEGACY,
            [](CraftRecipe* r, const std::string& value) -> bool
            {
                int v;
                if ( !enum_from_string_legacy(value, v, craft_output_mode_names) )
                    return false;

                r->output_mode = static_cast<CraftOutputMode>(v);
                return true;
            },
            [](CraftRecipe* r) -> std::string
            {
                return enum_to_string_legacy((int)r->output_mode, craft_output_mode_names);
            },
            "Output mode: create_proto or transform_base"
        ),

        make_olc_int_field<CraftRecipe>( "resultvnum", &CraftRecipe::result_vnum, "Prototype vnum used in create_proto mode", 0, INT_MAX),
        make_olc_custom_value_field<CraftRecipe>( "resulttype", OlcValueType::ENUM, (const void*)o_types, OlcMetaType::ENUM_FLAG,
            [](CraftRecipe* r, const std::string& value) -> bool { return set_enum_flag_field(r->result_item_type, value, o_types); },
            [](CraftRecipe* r) -> std::string { return get_enum_flag_field(r->result_item_type, o_types); },
            "Final object item type"
        ),

        make_olc_bool_field<CraftRecipe>( "customname", &CraftRecipe::allow_custom_name, "Whether the crafter may set a custom name"),
        make_olc_bool_field<CraftRecipe>( "extraargreq", &CraftRecipe::requires_extra_arg, "Whether this recipe requires an extra argument"),
        make_olc_std_string_field<CraftRecipe>( "extraarg", &CraftRecipe::extra_arg_name, "Name of the required extra argument"),
        make_olc_custom_value_field<CraftRecipe>( "wearflags", OlcValueType::FLAG, (const void*)w_flags, OlcMetaType::FLAG_TABLE,
            [](CraftRecipe* r, const std::string& value) -> bool { return craft_flag_vector_from_string(r->set_wear_flags, value, w_flags); },
            [](CraftRecipe* r) -> std::string { return craft_flag_vector_to_string(r->set_wear_flags, w_flags); },
            "Wear flags applied to the result object"
        ),

        make_olc_custom_value_field<CraftRecipe>( "objflags", OlcValueType::FLAG, (const void*)obj_flag_table, OlcMetaType::FLAG_TABLE,
            [](CraftRecipe* r, const std::string& value) -> bool { return craft_flag_vector_from_string(r->set_objflags, value, obj_flag_table); },
            [](CraftRecipe* r) -> std::string { return craft_flag_vector_to_string(r->set_objflags, obj_flag_table); },
            "Object flags applied to the result object"
        ),

        make_olc_std_string_field<CraftRecipe>( "nametemplate", &CraftRecipe::name_template, "Internal keyword/name template"),
        make_olc_std_string_field<CraftRecipe>( "shorttemplate", &CraftRecipe::short_template, "Short description template"),
        make_olc_std_string_field<CraftRecipe>( "desctemplate", &CraftRecipe::description_template, "Room description template"),
        make_olc_custom_editor_field<CraftRecipe>(
            "slots", OlcMetaType::LIST,
            [](CHAR_DATA* ch, CraftRecipe* recipe) -> bool { return olc_craft_edit_slots_field(ch, recipe); },
            [](CraftRecipe* recipe) -> std::string { return craft_slot_summary(recipe); },
            nullptr, 0, "Manage recipe slots. Type 'slots ?' for help.", true
        ),        
        make_olc_custom_editor_field<CraftRecipe>( "derivedvar", OlcMetaType::LIST,
            [](CHAR_DATA* ch, CraftRecipe* recipe) -> bool { return olc_craft_edit_derived_vars_field(ch, recipe); },
            [](CraftRecipe* recipe) -> std::string { return craft_derived_var_summary(recipe); },
            nullptr, 0, "Manage derived vars. Type 'derivedvar ?' for help.", true
        ),        
        make_olc_custom_editor_field<CraftRecipe>( "assignments", OlcMetaType::LIST,
            [](CHAR_DATA* ch, CraftRecipe* recipe) -> bool { return olc_craft_edit_assignments_field(ch, recipe); },
            [](CraftRecipe* recipe) -> std::string { return craft_assignment_summary(recipe); },
            nullptr, 0, "Manage result assignments. Type 'assignments ?' for help.", true
        ),        
        make_olc_custom_editor_field<CraftRecipe>( "affects", OlcMetaType::LIST,
            [](CHAR_DATA* ch, CraftRecipe* recipe) -> bool { return olc_craft_edit_affects_field(ch, recipe); },
            [](CraftRecipe* recipe) -> std::string { return craft_affect_summary(recipe); },
            nullptr, 0, "Manage recipe affects. Type 'affects ?' for help.", true
        ),        
        make_olc_bool_field<CraftRecipe>( "disabled", &CraftRecipe::disabled, "Whether the recipe is disabled for normal crafting use"),
        make_olc_custom_value_field<CraftRecipe>( "blockedwear", OlcValueType::FLAG, (const void*)w_flags, OlcMetaType::FLAG_TABLE,
            [](CraftRecipe* r, const std::string& value) -> bool { return craft_flag_vector_from_string(r->blocked_wear_flags, value, w_flags); },
            [](CraftRecipe* r) -> std::string { return craft_flag_vector_to_string(r->blocked_wear_flags, w_flags); },
            "Wear locations forbidden for recipes using extraarg wearloc"
        ),        
    };

    static OlcSchema<CraftRecipe> craft_schema;
    craft_schema.name = "craft";
    craft_schema.fields = &craft_fields;
    return &craft_schema;
}

// Value0 through 5 use description mapping, used for help files..
extern const OlcItemValueInfo g_olc_item_value_info[] =
{
    //Item Type  Used Vals_Used - {Value 0 - Value 5 use description}
    {ITEM_NONE, false, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_FIREWEAPON, false, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_ROPE, true, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_NOTE, false, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_BOAT, true, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_BLOOD, true, true, {nullptr, "quantity", nullptr, "poison strength", nullptr, nullptr}},
    {ITEM_HERB, true, true, {nullptr, "charges", "herb #", nullptr, nullptr, nullptr}},
    {ITEM_PULLCHAIN, true, true, {"flags - lever", "vnum/sn", "vnum", "vnum/value", nullptr, nullptr}},
    {ITEM_MATCH, false, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_TINDER, true, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_SHORT_BOW, false, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_SHOVEL, true, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_BATTERY, true, true, {"charges", nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_CIRCUIT, true, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_RARE_METAL, true, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_GRENADE, true, true, {"damage range - low", "damage range - high", nullptr, nullptr, nullptr, nullptr}},
    {ITEM_FIGHTERCOMP, true, true, {"condition", "type", "size dice", "value", nullptr, nullptr}},
    {ITEM_DISGUISE, true, true, {"max condition", "condition", "race", "sex", nullptr, nullptr}},
    {ITEM_CARGO, true, true, {"type", "quantity", nullptr, nullptr, nullptr, nullptr}},
    {ITEM_LIGHT, true, true, {nullptr, nullptr, "timer", nullptr, nullptr, nullptr}},
    {ITEM_MISSILE, false, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_FURNITURE, true, true, {"capacity", nullptr, "type (at=1,on=2,in=3", nullptr, nullptr, nullptr}},
    {ITEM_DRINK_CON, true, true, {"capacity", "quantity", "liquid #", "poison?", nullptr, nullptr}},
    {ITEM_CORPSE_NPC, true, true, {nullptr, nullptr, "timer", nullptr, nullptr, nullptr}},
    {ITEM_BLOODSTAIN, true, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_INCENSE, false, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_BUTTON, true, true, {"flags - lever", "vnum/sn", "vnum", "vnum/value", nullptr, nullptr}},
    {ITEM_TRAP, true, true, {"charges", "type", "level", "flags", nullptr, nullptr}},
    {ITEM_LOCKPICK, false, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_LONG_BOW, false, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_SALVE, true, true, {"power level", "max charge", "charges", "delay", "sn", "sn"}},
    {ITEM_TOOLKIT, true, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_SUPERCONDUCTOR, true, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_MAGNET, false, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_LANDMINE, true, true, {"damage range - low", "damage range - high", nullptr, nullptr, nullptr, nullptr}},
    {ITEM_MIDCOMP, true, true, {"condition", "type", "size dice", "value", nullptr, nullptr}},
    {ITEM_DIS_FABRIC, true, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_TRACKINGDEVICE, false, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_SCROLL, true, true, {"power level", "sn", "sn", "sn", nullptr, nullptr}},
    {ITEM_TREASURE, true, true, {"type", "condition", nullptr, nullptr, nullptr, nullptr}},
    {ITEM_TRASH, true, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_KEY, true, true, {"lock#", nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_CORPSE_PC, true, true, {nullptr, nullptr, "timer", nullptr, nullptr, nullptr}},
    {ITEM_SCRAPS, true, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_FIRE, true, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_DIAL, false, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_MAP, false, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_SPIKE, false, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_CROSSBOW, true, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_RAWSPICE, true, true, {"type", "power", nullptr, nullptr, nullptr, nullptr}},
    {ITEM_DURASTEEL, true, true, {"current AC", "original AC", nullptr, nullptr, nullptr, nullptr}},
    {ITEM_COMLINK, true, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_THREAD, true, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_GOVERNMENT, false, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_CAPITALCOMP, true, true, {"condition", "type", "size dice", "value", nullptr, nullptr}},
    {ITEM_HAIR, true, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_WAND, true, true, {"power level", "max charges", "charges", "sn", nullptr, nullptr}},
    {ITEM_ARMOR, true, true, {"current AC", "original AC", "", "timer", nullptr, nullptr}},
    {ITEM_OLDTRAP, false, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_FOOD, true, true, {"food value", "condition", "", "poison strength", nullptr, nullptr}},
    {ITEM_FOUNTAIN, true, true, {"capacity", "quantity", nullptr, nullptr, nullptr, nullptr}},
    {ITEM_PIPE, true, true, {"capacity", "quantity", nullptr, nullptr, nullptr, nullptr}},
    {ITEM_BOOK, true, true, {"power level", "sn", "sn", "sn", nullptr, nullptr}},
    {ITEM_RUNE, false, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_PORTAL, false, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_DISEASE, false, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_AMMO, true, true, {"charges", nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_LENS, true, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_OVEN, true, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_MEDPAC, true, true, {"charges", nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_SPICE, true, true, {"type", "power", nullptr, nullptr, nullptr, nullptr}},
    {ITEM_DROID_CORPSE, true, true, {nullptr, nullptr, "timer", nullptr, nullptr, nullptr}},
    {ITEM_CHEMICAL, true, true, {"strength", nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_STUNGRENADE, false, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_STAFF, true, true, {"power level", "max charges", "charges", "sn", "", nullptr}},
    {ITEM_POTION, true, true, {"power level", "sn", "sn", "sn", "", nullptr}},
    {ITEM_CONTAINER, true, true, {"capacity", "flags - container", "key vnum", "condition", nullptr, nullptr}},
    {ITEM_MONEY, true, true, {"quantity", "type", "", "", nullptr, nullptr}},
    {ITEM_PILL, true, true, {"power level", "sn", "sn", "sn", "food val", nullptr}},
    {ITEM_HERB_CON, true, true, {"capacity", "quantity", "type (herb)", nullptr, nullptr, nullptr}},
    {ITEM_SWITCH, true, true, {"flags - lever", "vnum/sn", "vnum", "vnum/value", "", nullptr}},
    {ITEM_RUNEPOUCH, false, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_PAPER, true, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_OIL, true, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_QUIVER, false, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_CRYSTAL, true, true, {"type (power)", nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_MIRROR, true, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_FABRIC, true, true, {"current AC", "original AC", nullptr, nullptr, nullptr, nullptr}},
    {ITEM_SMUT, true, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_BOLT, true, true, {"charges", nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_WEAPON, true, true, {"condition", "num dice", "size dice", "weapontype", "Current Charge", "Max Charge"}},
    {ITEM_PEN, true, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_LEVER, true, true, {"flags - lever", "vnum/sn", "vnum", "vnum/value", nullptr, nullptr}},
    {ITEM_FUEL, false, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_DURAPLAST, true, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_DEVICE, true, true, {"power level", "max charges", "charges", "sn", nullptr, nullptr}},
    {ITEM_SCOPE, true, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {ITEM_SPACECRAFT, true, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {-1, false, false, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}}
};

extern const size_t g_olc_item_value_info_count =
    sizeof(g_olc_item_value_info) / sizeof(g_olc_item_value_info[0]);


// --------------------------------------------
// ROOM_OLC room_olc OlcOps functions
// --------------------------------------------
void* room_olc_clone(const void* src)
{
    return olc_room_clone(static_cast<const ROOM_INDEX_DATA*>(src));
}

void room_olc_free_clone(void* obj)
{
    olc_room_free(static_cast<ROOM_INDEX_DATA*>(obj));
}

void room_olc_apply_changes(void* original, void* working)
{
    olc_room_apply_changes(
        static_cast<ROOM_INDEX_DATA*>(original),
        static_cast<ROOM_INDEX_DATA*>(working)
    );
}

void room_olc_save(CHAR_DATA* ch, void* original)
{
    if (!ch || !original)
        return;

    ROOM_INDEX_DATA* room = static_cast<ROOM_INDEX_DATA*>(original);
    AREA_DATA* area = room->area;

    if (!area)
    {
        send_to_char("No area associated.\n", ch);
        return;
    }
//  To be consistent, taking out the area save for rooms since i'm not doing it for mobs/objects - DV 4-5-26
//  fold_area(area, area->filename, FALSE);
//  send_to_char("Area saved.\n", ch);
}

void room_olc_after_commit(CHAR_DATA* ch, void* original, void* /*working*/)
{
    ROOM_INDEX_DATA* live = static_cast<ROOM_INDEX_DATA*>(original);
    if (!live)
        return;

    olc_room_relink_exits(live);

    if (!ch || !ch->desc || !ch->desc->olc)
        return;

    auto sess = ch->desc->olc;

    for (const auto& p : sess->pending_exit_side_effects)
        olc_room_apply_pending_exit_side_effect(p);

    sess->pending_exit_side_effects.clear();
}

void room_olc_after_revert(CHAR_DATA* ch, void* /*original*/, void* /*working*/)
{
    if (!ch || !ch->desc || !ch->desc->olc)
        return;

    ch->desc->olc->pending_exit_side_effects.clear();
}

const OlcOps room_olc_ops =
{
    room_olc_clone,
    room_olc_free_clone,
    room_olc_apply_changes,
    room_olc_save,
    room_olc_after_commit,
    room_olc_after_revert,
    olc_room_edit_interpret,
    OlcInterpretStage::LATE    
};

// --------------------------------------------
// OLC_ROOM olc_room specific functions
// --------------------------------------------
bool olc_room_edit_revert(CHAR_DATA* ch)
{
    if (!ch || !ch->desc || !ch->desc->olc)
        return false;

    auto sess = ch->desc->olc;

    /* Must be editing a room */
    if (!sess->schema || !sess->schema->name ||
        str_cmp(sess->schema->name, "room"))
        return false;

    if (!sess->original_clone || !sess->ops ||
        !sess->ops->clone || !sess->ops->free_clone)
    {
        send_to_char("No revert snapshot is available.\n", ch);
        return false;
    }

    /* Free current working copy */
    if (sess->working_copy)
        sess->ops->free_clone(sess->working_copy);

    /* Restore from original clone */
    sess->working_copy = sess->ops->clone(sess->original_clone);
    if (!sess->working_copy)
    {
        send_to_char("Failed to restore revert snapshot.\n", ch);
        return false;
    }

    sess->last_cmd_arg.clear();
    sess->dirty = false;

    /* IMPORTANT: clear pending exit side effects */
    sess->pending_exit_side_effects.clear();

    if (sess->ops->after_revert)
        sess->ops->after_revert(ch, sess->original, sess->working_copy);

    send_to_char("Room reverted to pre-edit state.\n", ch);
    return true;
}

EXTRA_DESCR_DATA* olc_room_find_extra_desc(ROOM_INDEX_DATA* room, const std::string& keyword)
{
    if (!room || keyword.empty())
        return nullptr;

    EXTRA_DESCR_DATA* best_match = nullptr;
    MatchResult best_result;

    for (auto ed = room->first_extradesc; ed; ed = ed->next)
    {
        if (!ed->keyword)
            continue;

        MatchResult result = match_keywords(ed->keyword, keyword);

        // Exact match wins immediately
        if (result.type == MatchType::EXACT)
            return ed;

        // Keep best prefix match
        if (result.type == MatchType::PREFIX &&
            result.match_length > best_result.match_length)
        {
            best_result = result;
            best_match = ed;
        }
    }

    return best_match;
}

EXTRA_DESCR_DATA* olc_room_get_or_create_extra_desc(ROOM_INDEX_DATA* room, const std::string& keyword)
{
    auto ed = olc_room_find_extra_desc(room, keyword);

    if (ed)
        return ed;

    ed = new EXTRA_DESCR_DATA();
    ed->keyword = STRALLOC(keyword);
    ed->description = STRALLOC("");

    ed->next = room->first_extradesc;
    room->first_extradesc = ed;

    return ed;
}

EXIT_DATA* olc_room_clone_exits(EXIT_DATA* src)
{
    EXIT_DATA* first = nullptr;
    EXIT_DATA* last = nullptr;

    for (; src; src = src->next)
    {
        EXIT_DATA* ex = new EXIT_DATA{};

        ex->prev = last;
        ex->next = nullptr;
        ex->rexit = nullptr;

        ex->to_room = src->to_room;
        ex->keyword = src->keyword ? STRALLOC(src->keyword) : nullptr;
        ex->description = src->description ? STRALLOC(src->description) : nullptr;
        ex->vnum = src->vnum;
        ex->rvnum = src->rvnum;
        ex->exit_info = src->exit_info;
        ex->key = src->key;
        ex->vdir = src->vdir;
        ex->distance = src->distance;

        if (!first)
            first = ex;
        else
            last->next = ex;

        last = ex;
    }

    return first;
}

ROOM_INDEX_DATA* olc_room_clone(const ROOM_INDEX_DATA* src)
{
    if (!src)
        return nullptr;

    ROOM_INDEX_DATA* dst = new ROOM_INDEX_DATA{};

    dst->sector_type = src->sector_type;
    dst->room_flags  = src->room_flags;
    dst->tunnel      = src->tunnel;
    dst->light       = src->light;
    dst->tele_vnum   = src->tele_vnum;
    dst->tele_delay  = src->tele_delay;

    // Non editable field - used in olc show only
    dst->vnum        = src->vnum;
    dst->area        = src->area;

    // Fix pointer fields

    dst->name = src->name ? str_dup(src->name) : nullptr;
    dst->description = src->description ? STRALLOC(src->description) : nullptr;

    // Do NOT carry live world state
    dst->first_person = nullptr;
    dst->last_person = nullptr;
    dst->first_content = nullptr;
    dst->last_content = nullptr;

    dst->first_exit = olc_room_clone_exits(src->first_exit);
    dst->last_exit = nullptr;
    for (auto ex = dst->first_exit; ex; ex = ex->next)
        dst->last_exit = ex;

    dst->first_extradesc = olc_room_clone_extra_descs(src->first_extradesc);

    // rebuild last pointer
    dst->last_extradesc = nullptr;
    for (auto ed = dst->first_extradesc; ed; ed = ed->next)
        dst->last_extradesc = ed;

    dst->mudprogs = nullptr;
    dst->mpact = nullptr;

    return dst;
}

bool olc_room_remove_extra_desc(ROOM_INDEX_DATA* room, const char* keyword)
{
    for (auto ed = room->first_extradesc; ed; ed = ed->next)
    {
        if (is_name(keyword, ed->keyword))
        {
            // unlink
            if (ed->prev)
                ed->prev->next = ed->next;
            else
                room->first_extradesc = ed->next;

            if (ed->next)
                ed->next->prev = ed->prev;
            else
                room->last_extradesc = ed->prev;

            // free
            if (ed->keyword)
                STRFREE(ed->keyword);
            if (ed->description)
                STRFREE(ed->description);

            DISPOSE(ed);
            return true;
        }
    }

    return false;
}

EXTRA_DESCR_DATA* olc_room_clone_extra_descs(EXTRA_DESCR_DATA* src)
{
    EXTRA_DESCR_DATA* first = nullptr;
    EXTRA_DESCR_DATA* last  = nullptr;

    for (; src; src = src->next)
    {
        auto ed = new EXTRA_DESCR_DATA{};

        ed->keyword     = src->keyword ? STRALLOC(src->keyword) : nullptr;
        ed->description = src->description ? STRALLOC(src->description) : nullptr;

        ed->next = nullptr;
        ed->prev = last;

        if (!first)
            first = ed;
        else
            last->next = ed;

        last = ed;
    }

    return first;
}

void olc_room_free_extra_descs(EXTRA_DESCR_DATA* ed)
{
    while (ed)
    {
        auto next = ed->next;

        if (ed->keyword)
            STRFREE(ed->keyword);

        if (ed->description)
            STRFREE(ed->description);

        DISPOSE(ed);
        ed = next;
    }
}

void olc_room_free_exits( ROOM_INDEX_DATA *room, EXIT_DATA *xit )
{
    EXIT_DATA *exit, *exit_next;
    if (!xit)
        return;

    for ( exit = xit; exit; exit = exit_next )
    {
        exit_next = exit->next;
        olc_room_delete_exit(room, exit);
    }

}

void olc_room_free(ROOM_INDEX_DATA* room)
{
    if (!room)
        return;

    if (room->name)
        STR_DISPOSE(room->name);

    if (room->description)
        STRFREE(room->description);

    if (room->first_extradesc)
        olc_room_free_extra_descs(room->first_extradesc);

    if (room->first_exit)
        olc_room_free_exits(room, room->first_exit);

    room->first_extradesc = nullptr;
    room->last_extradesc  = nullptr;
    room->first_exit      = nullptr;
    room->last_exit       = nullptr;

    delete room;
}

void olc_room_apply_changes(ROOM_INDEX_DATA* src, ROOM_INDEX_DATA* dst)
{
    // Strings
    if (src->name)
        STR_DISPOSE(src->name);
    src->name = dst->name ? str_dup(dst->name) : nullptr;

    if (src->description)
        STRFREE(src->description);
    src->description = dst->description ? STRALLOC(dst->description) : nullptr;

    // Scalars
    src->sector_type = dst->sector_type;
    src->room_flags  = dst->room_flags;
    src->tunnel      = dst->tunnel;
    src->light       = dst->light;
    src->tele_vnum   = dst->tele_vnum;
    src->tele_delay  = dst->tele_delay;

    // Extra descriptions
    if (src->first_extradesc)
        olc_room_free_extra_descs(src->first_extradesc);

    src->first_extradesc = olc_room_clone_extra_descs(dst->first_extradesc);
    src->last_extradesc = nullptr;
    for (auto ed = src->first_extradesc; ed; ed = ed->next)
        src->last_extradesc = ed;

    // Exits
    if (src->first_exit)
        olc_room_free_exits(src, src->first_exit);

    src->first_exit = olc_room_clone_exits(dst->first_exit);
    src->last_exit = nullptr;
    for (auto ex = src->first_exit; ex; ex = ex->next)
        src->last_exit = ex;
}

bool olc_room_finish_extradesc_edit(
    CHAR_DATA* ch, ROOM_INDEX_DATA* room, const std::string& value )
{
    EXTRA_DESCR_DATA *ed = nullptr;

    (void)room;

    if ( !ch || !ch->desc || !ch->desc->olc || !ch->desc->olc->editor_context )
    {
        bug( "olc_room_finish_extradesc_edit: NULL editor_context", 0 );
        return false;
    }

    ed = static_cast<EXTRA_DESCR_DATA*>( ch->desc->olc->editor_context );
    ch->desc->olc->editor_context = nullptr;

    return set_str_field( ed->description, value );
}

bool olc_room_edit_extradesc_field(CHAR_DATA* ch, ROOM_INDEX_DATA* room)
{
    const char* arg = ch->desc->olc->last_cmd_arg.c_str();

    if (!arg || arg[0] == '\0')
    {
        olc_show_extradesc_help(ch);
        return false;
    }

    /* DELETE SUPPORT */
    if (arg[0] == '-')
    {
        const char* key = arg + 1;

        if (!key || key[0] == '\0')
        {
            send_to_char("Delete which keyword?\n", ch);
            return false;
        }

        if (olc_room_remove_extra_desc(room, key))
        {
            send_to_char("Extra description deleted.\n", ch);
            ch->desc->olc->dirty = true;
        }
        else
        {
            send_to_char("No such extra description.\n", ch);
        }

        return true;
    }

    /* CREATE / EDIT */
    auto ed = olc_room_get_or_create_extra_desc(room, arg);

    ch->substate = SUB_ROOM_EXTRA;
    ch->dest_buf = room;
    ch->last_cmd = do_olcset;

    ch->desc->olc->editor_context = ed;    
    start_editing(ch, ed->description);

    return true;
}

bool olc_room_parse_exit_create_options(
    CHAR_DATA* ch,
    std::istringstream& iss,
    bool default_two_way,
    OlcExitCreateOptions& out)
{
    out.two_way = default_two_way;

    std::string next;
    if (!(iss >> next))
        return true;

    /* optional "two" first */
    if (!str_prefix(next, "two"))
    {
        out.two_way = true;

        if (!(iss >> next))
            return true;
    }

    /* if anything remains now, it must begin with flags bitmask */
    int flags = 0;
    if (!olc_format_parse_strict_int(next, flags) || flags < 0)
    {
        send_to_char("When providing create options, the first extra value must be a numeric flags bitmask.\n", ch);
        send_to_char("Flags bitmask can be 0 (nothing set)\n", ch);        
        send_to_char("Usage: exit <dir> <vnum> [two] [flags] [key] [keyword]\n", ch);
        return false;
    }

    out.has_flags = true;
    out.flags = flags;

    std::string keytok;
    if (!(iss >> keytok))
        return true;

    int key = 0;
    if (!olc_format_parse_strict_int(keytok, key))
    {
        send_to_char("When providing a keyword on create, you must also provide a numeric key first.\n", ch);
        send_to_char("Usage: exit <dir> <vnum> [two] [flags] [key] [keyword]\n", ch);
        return false;
    }

    out.has_key = true;
    out.key = key;

    std::string rest;
    getline(iss, rest);
    rest = olc_format_trim_left(rest);

    if (!rest.empty())
    {
        out.has_keyword = true;
        out.keyword = rest;
    }

    return true;
}

void olc_room_apply_create_options_to_exit(EXIT_DATA* ex, const OlcExitCreateOptions& opts)
{
    if (!ex)
        return;

    if (opts.has_flags)
        ex->exit_info = opts.flags;

    if (opts.has_key)
        ex->key = opts.key;

    if (opts.has_keyword)
        set_str_field(ex->keyword, opts.keyword);
}

void olc_room_apply_create_options_to_pending_reverse(
    OlcPendingExitSideEffect* p,
    const OlcExitCreateOptions& opts)
{
    if (!p)
        return;

    p->type = OlcExitSideEffectType::UPSERT_REVERSE;

    if (opts.has_flags)
        p->final_exit_info = opts.flags;

    if (opts.has_key)
        p->final_key = opts.key;

    if (opts.has_keyword)
        p->final_keyword = opts.keyword;
}

OlcExitSelector olc_room_resolve_exit_selector(ROOM_INDEX_DATA* room, const std::string& token)
{
    OlcExitSelector result;

    if (!room || token.empty())
        return result;

    /* #3 = third exit in room's exit list */
    if (token[0] == '#')
    {
        int index = atoi(token.c_str() + 1);
        result.ex = find_exit_by_number(room, index);
        if (result.ex)
        {
            result.dir = result.ex->vdir;
            result.has_dir = true;
        }
        return result;
    }

    /* 2.east / 2.? / 2.somewhere */
    std::string::size_type dot = token.find('.');
    if (dot != std::string::npos)
    {
        std::string left = token.substr(0, dot);
        std::string right = token.substr(dot + 1);

        if (!left.empty() && !right.empty())
        {
            int index = atoi(left.c_str());
            int dir = dir_lookup(right);
            if (index > 0 && dir >= 0)
            {
                result.ex = find_exit_by_dir_index(room, dir, index);
                result.dir = dir;
                result.has_dir = true;
                return result;
            }
        }
    }

    /* plain direction */
    int dir = dir_lookup(token);
    if (dir >= 0)
    {
        result.ex = find_exit(room, dir);
        result.dir = dir;
        result.has_dir = true;
        return result;
    }

    /* keyword */
    result.ex = find_exit_by_keyword(room, token);
    if (result.ex)
    {
        result.dir = result.ex->vdir;
        result.has_dir = true;
    }

    return result;
}

void olc_room_relink_exits(ROOM_INDEX_DATA* room)
{
    if (!room)
        return;

    EXIT_DATA* ex;
    EXIT_DATA* ex_next;
    bool has_exit = false;

    /* First pass: refresh destination pointers, rvnum, and remove bad exits */
    for (ex = room->first_exit; ex; ex = ex_next)
    {
        ex_next = ex->next;

        ex->rvnum = room->vnum;
        ex->rexit = nullptr;

        if (ex->vnum <= 0 || (ex->to_room = get_room_index(ex->vnum)) == nullptr)
        {
            bug("olc_room_relink_exits: deleting bad %s exit in room %d (vnum %d)",
                exit_dir_label(ex->vdir), room->vnum, ex->vnum);
            extract_exit(room, ex);
            continue;
        }

        has_exit = true;
    }

    if (!has_exit)
        BV_SET_BIT(room->room_flags, ROOM_NO_MOB);

    /* Second pass: rebuild reverse pointers */
    for (ex = room->first_exit; ex; ex = ex->next)
    {
        if (!ex->to_room)
            continue;

        int rdir = get_rev_dir(ex->vdir);
        if (rdir < 0)
            continue;

        EXIT_DATA* rev_exit = get_exit_to(ex->to_room, rdir, room->vnum);
        if (rev_exit)
        {
            ex->rexit = rev_exit;
            rev_exit->rexit = ex;
        }
    }
}

void olc_room_update_exit_destination(EXIT_DATA* ex, int vnum)
{
    if (!ex)
        return;

    ex->vnum = vnum;
    ex->to_room = get_room_index(vnum);
}

EXIT_DATA* olc_room_create_exit(ROOM_INDEX_DATA* room, int dir, int vnum)
{
    if (!room)
        return nullptr;

    ROOM_INDEX_DATA* to_room = get_room_index(vnum);
    EXIT_DATA* ex = make_exit(room, to_room, static_cast<sh_int>(dir));

    if (!ex)
        return nullptr;

   ex->keyword		= STRALLOC( "" );
	ex->description	= STRALLOC( "" );
	ex->key		= -1;
	ex->distance = 1;
    ex->exit_info = 0;


    if (!to_room)
        ex->vnum = vnum;

    return ex;
}

bool olc_room_delete_exit(ROOM_INDEX_DATA* room, EXIT_DATA* ex)
{
    if (!room || !ex)
        return false;
        
    extract_exit(room, ex);
    return true;
}

EXIT_DATA* olc_room_find_reverse_exit_to_room(ROOM_INDEX_DATA* room, int dir, int to_vnum)
{
    if (!room || dir < 0)
        return nullptr;

    for (EXIT_DATA* ex = room->first_exit; ex; ex = ex->next)
    {
        if (ex->vdir != dir)
            continue;

        if (ex->vnum == to_vnum)
            return ex;
    }

    return nullptr;
}

void olc_room_init_pending_reverse_from_live(
    OlcPendingExitSideEffect& p,
    ROOM_INDEX_DATA* from,
    EXIT_DATA* ex)
{
    p.final_vnum = from->vnum;
    p.final_rvnum = ex->vnum;
    p.final_key = -1;
    p.final_exit_info = 0;
    p.final_keyword.clear();

    if (!ex->to_room)
    {
        p.initialized = true;
        return;
    }

    int rdir = get_rev_dir(ex->vdir);
    if (rdir < 0)
    {
        p.initialized = true;
        return;
    }

    EXIT_DATA* back = olc_room_find_reverse_exit_to_room(ex->to_room, rdir, from->vnum);
    if (back)
    {
        p.final_key = back->key;
        p.final_exit_info = back->exit_info;
        p.final_keyword = (back->keyword && back->keyword[0]) ? back->keyword : "";
    }

    p.initialized = true;
}

OlcPendingExitSideEffect* olc_room_get_or_create_pending_reverse_exit(
    CHAR_DATA* ch,
    ROOM_INDEX_DATA* from,
    EXIT_DATA* ex)
{
    if (!ch || !ch->desc || !ch->desc->olc || !from || !ex)
        return nullptr;

    if (ex->vdir < 0 || ex->vdir >= MAX_DIR+1)
        return nullptr;

    if (ex->vnum <= 0)
        return nullptr;

    auto& pending = ch->desc->olc->pending_exit_side_effects;

    for (auto& p : pending)
    {
        if (p.from_room_vnum == from->vnum &&
            p.from_dir == ex->vdir &&
            p.to_room_vnum == ex->vnum)
        {
            if (!p.initialized)
                olc_room_init_pending_reverse_from_live(p, from, ex);
            return &p;
        }
    }

    OlcPendingExitSideEffect p;
    p.type = OlcExitSideEffectType::UPSERT_REVERSE;
    p.from_room_vnum = from->vnum;
    p.from_dir = ex->vdir;
    p.to_room_vnum = ex->vnum;

    olc_room_init_pending_reverse_from_live(p, from, ex);

    pending.push_back(p);
    return &pending.back();
}

void olc_room_queue_reverse_delete(
    CHAR_DATA* ch,
    ROOM_INDEX_DATA* from,
    EXIT_DATA* ex)
{
    OlcPendingExitSideEffect* p = olc_room_get_or_create_pending_reverse_exit(ch, from, ex);
    if (!p)
        return;

    p->type = OlcExitSideEffectType::DELETE_REVERSE;
}

void olc_room_queue_reverse_upsert(
    CHAR_DATA* ch,
    ROOM_INDEX_DATA* from,
    EXIT_DATA* ex)
{
    OlcPendingExitSideEffect* p = olc_room_get_or_create_pending_reverse_exit(ch, from, ex);
    if (!p)
        return;

    p->type = OlcExitSideEffectType::UPSERT_REVERSE;
}

void olc_room_apply_pending_reverse_exit_delete(const OlcPendingExitSideEffect& p)
{
    ROOM_INDEX_DATA* from = get_room_index(p.from_room_vnum);
    ROOM_INDEX_DATA* to   = get_room_index(p.to_room_vnum);

    if (!from || !to)
        return;

    int rdir = get_rev_dir(p.from_dir);
    if (rdir < 0)
        return;

    EXIT_DATA* back = olc_room_find_reverse_exit_to_room(to, rdir, from->vnum);
    if (!back)
        return;

    extract_exit(to, back);
}

void olc_room_apply_pending_reverse_exit_upsert(const OlcPendingExitSideEffect& p)
{
    ROOM_INDEX_DATA* from = get_room_index(p.from_room_vnum);
    ROOM_INDEX_DATA* to   = get_room_index(p.to_room_vnum);

    if (!from || !to)
        return;

    int rdir = get_rev_dir(p.from_dir);
    if (rdir < 0)
        return;

    EXIT_DATA* ex = get_exit(from, p.from_dir);
    if (!ex)
        return;

    EXIT_DATA* back = olc_room_find_reverse_exit_to_room(to, rdir, from->vnum);
    if (!back)
    {
        back = olc_room_create_exit(to, rdir, from->vnum);
        if (!back)
            return;
    }

    back->vnum = p.final_vnum;
    back->rvnum = p.final_rvnum;
    back->to_room = from;
    back->key = p.final_key;
    back->exit_info = p.final_exit_info;

    if (back->keyword)
        STRFREE(back->keyword);
    back->keyword = STRALLOC(p.final_keyword);

    ex->rexit = back;
    back->rexit = ex;
}

void olc_room_apply_pending_exit_side_effect(const OlcPendingExitSideEffect& p)
{
    switch (p.type)
    {
        case OlcExitSideEffectType::UPSERT_REVERSE:
            olc_room_apply_pending_reverse_exit_upsert(p);
            break;

        case OlcExitSideEffectType::DELETE_REVERSE:
            olc_room_apply_pending_reverse_exit_delete(p);
            break;
    }
}

void olc_room_exit_link_two_way(EXIT_DATA* ex, ROOM_INDEX_DATA* from)
{
    if (!ex || !from || !ex->to_room)
        return;

    int rdir = get_rev_dir(ex->vdir);
    if (rdir < 0)
        return;

    ROOM_INDEX_DATA* to = ex->to_room;

    /* Find a reverse-direction exit that actually leads back to the source room */
    EXIT_DATA* back = olc_room_find_reverse_exit_to_room(to, rdir, from->vnum);

    if (!back)
    {
        back = olc_room_create_exit(to, rdir, from->vnum);
        if (!back)
            return;
    }
    else
    {
        back->vnum = from->vnum;
        back->rvnum = to->vnum;
        back->to_room = from;
    }

    ex->rexit = back;
    back->rexit = ex;
}

const char* olc_room_match_exit_command(const std::string& input)
{
    const char* cmds[] =
    {
        "delete",
        "flags",
        "key",
        "desc",
        "keyword",
        nullptr
    };

    if (input.empty())
        return nullptr;

    /* Exact match wins immediately */
    for (int i = 0; cmds[i] != nullptr; ++i)
    {
        if (!str_cmp(input, cmds[i]))
            return cmds[i];
    }

    /* Otherwise, find unique prefix match */
    const char* match = nullptr;

    for (int i = 0; cmds[i] != nullptr; ++i)
    {
        if (!str_prefix(input, cmds[i]))
        {
            if (match)
                return nullptr; /* ambiguous */
            match = cmds[i];
        }
    }

    return match;
}

bool olc_room_exit_command_is_ambiguous(const std::string& input)
{
    const char* cmds[] =
    {
        "delete",
        "flags",
        "key",
        "desc",
        "keyword",
        nullptr
    };

    if (input.empty())
        return false;

    int matches = 0;

    for (int i = 0; cmds[i] != nullptr; ++i)
    {
        if (!str_prefix(input, cmds[i]))
            ++matches;
    }

    return matches > 1;
}

void olc_room_show_ambiguous_exit_command(CHAR_DATA* ch, const std::string& input)
{
    const char* cmds[] =
    {
        "delete",
        "flags",
        "key",
        "desc",
        "keyword",
        nullptr
    };

    send_to_char(("Ambiguous exit command: " + input + "\n"), ch);
    send_to_char("Matches:\n", ch);

    for (int i = 0; cmds[i] != nullptr; ++i)
    {
        if (!str_prefix(input, cmds[i]))
        {
            send_to_char("  ", ch);
            send_to_char(cmds[i], ch);
            send_to_char("\n", ch);
        }
    }
}

std::string olc_room_extradesc_list_summary(void* obj)
{
    auto room = static_cast<ROOM_INDEX_DATA*>(obj);
    std::string out;

    for (auto ed = room->first_extradesc; ed; ed = ed->next)
    {
        if (ed->keyword && ed->keyword[0] != '\0')
        {
            if (!out.empty())
                out += " ";
            out += ed->keyword;
        }
    }

    return out.empty() ? "none" : out;
}

std::string olc_room_exit_list_summary(void* obj)
{
    auto room = static_cast<ROOM_INDEX_DATA*>(obj);
    if (!room)
        return "none";

    std::string out;
    int exit_num = 0;
    int dir_counts[MAX_DIR+1];

    for (int i = 0; i <= MAX_DIR; ++i)
        dir_counts[i] = 0;

    for (auto ex = room->first_exit; ex; ex = ex->next)
    {
        ++exit_num;

        int dir = ex->vdir;
        bool valid_dir = (dir >= 0 && dir <= MAX_DIR+1);

        int dir_index = 0;
        if (valid_dir)
            dir_index = ++dir_counts[dir];

        if (!out.empty())
            out += "  ";

        out += "#";
        out += std::to_string(exit_num);
        out += "=";

        if (valid_dir)
        {
            out += exit_dir_label(dir);
        }
        else
        {
            out += "?";
        }

        if (valid_dir && dir_index > 1)
        {
            out += "/";
            out += std::to_string(dir_index);
            out += ".";

            out += exit_dir_label(dir);
        }

        if (ex->keyword && ex->keyword[0] != '\0')
        {
            out += "/";
            out += ex->keyword;
        }
    }

    return out.empty() ? "none" : out;
}

bool olc_room_handle_exit_delete(CHAR_DATA* ch, ROOM_INDEX_DATA* room, EXIT_DATA* ex, bool two_way)
{
    if (!room || !ex)
    {
        send_to_char("No such exit.\n", ch);
        return false;
    }

    if (two_way)
        olc_room_queue_reverse_delete(ch, room, ex);

    if (olc_room_delete_exit(room, ex))
    {
        send_to_char(two_way ? "Exit deleted. Reverse exit queued for deletion on save.\n"
                             : "Exit deleted.\n", ch);
        ch->desc->olc->dirty = true;
    }
    else
    {
        send_to_char("No such exit.\n", ch);
    }

    return true;
}

bool olc_room_handle_exit_create(
    CHAR_DATA* ch,
    ROOM_INDEX_DATA* room,
    int dir,
    int vnum,
    bool two_way,
    bool force_new,
    bool reverse_default,
    const OlcExitCreateOptions& opts)
{
    EXIT_DATA* ex = nullptr;

    if (!force_new)
        ex = find_exit(room, dir);

    if (!ex)
    {
        ex = olc_room_create_exit(room, dir, vnum);
        if (!ex)
        {
            send_to_char("Failed to create exit.\n", ch);
            return false;
        }

        send_to_char("Exit created.\n", ch);
    }
    else
    {
        olc_room_update_exit_destination(ex, vnum);
        send_to_char("Exit updated.\n", ch);
    }

    olc_room_apply_create_options_to_exit(ex, opts);

    if (reverse_default)
    {
        OlcPendingExitSideEffect* p = nullptr;

        if (two_way)
        {
            p = olc_room_get_or_create_pending_reverse_exit(ch, room, ex);
            if (!p)
            {
                send_to_char("Unable to queue reverse exit changes.\n", ch);
                return false;
            }

            p->type = OlcExitSideEffectType::UPSERT_REVERSE;
            olc_room_apply_create_options_to_pending_reverse(p, opts);
        }
    }
    else
    {
        if (two_way)
            olc_room_queue_reverse_upsert(ch, room, ex);
    }

    ch->desc->olc->dirty = true;
    return true;
}

bool olc_room_handle_exit_flags(CHAR_DATA* ch, EXIT_DATA* ex, const std::string& flags)
{
    if (flags.empty())
    {
        send_to_char("No flags specified.\n", ch);

        std::string col = olc_format_exit_flags_list();
        if (!col.empty())
        {
            send_to_char("Valid flags:\n", ch);
            send_to_char(col, ch);
        }

        send_to_char("Usage: exit <dir> flags <flag> [flag]...\n", ch);
        return false;
    }

    int bits = ex->exit_info;
    std::istringstream fss(flags);
    std::string word;

    while (fss >> word)
    {
        bool remove = false;

        if (!word.empty() && word[0] == '-')
        {
            remove = true;
            word = word.substr(1);
        }

        int bit = get_exflag(word);
        if (bit < 0)
        {
            send_to_char(("Unknown flag: " + word + "\n"), ch);

            std::string col = olc_format_exit_flags_list();
            if (!col.empty())
            {
                send_to_char("Valid flags:\n", ch);
                send_to_char(col, ch);
            }

            return false;
        }

        if (remove)
            REMOVE_BIT(bits, (1 << bit));
        else
            SET_BIT(bits, (1 << bit));
    }

    ex->exit_info = bits;
    send_to_char("Exit flags updated.\n", ch);
    ch->desc->olc->dirty = true;
    return true;
}

bool olc_room_handle_exit_key(CHAR_DATA* ch, EXIT_DATA* ex, std::istringstream& iss)
{
    int key = 0;

    if (!(iss >> key))
    {
        send_to_char("Invalid key vnum.\n", ch);
        send_to_char("Usage: exit <dir> key <vnum>\n", ch);
        return false;
    }

    ex->key = key;
    send_to_char("Exit key set.\n", ch);
    ch->desc->olc->dirty = true;
    return true;
}

std::string olc_room_exit_remaining_args(const std::string& arg)
{
    std::istringstream iss(arg);
    std::string word1, word2;

    iss >> word1 >> word2;

    std::string rest;
    getline(iss, rest);

    size_t pos = rest.find_first_not_of(' ');
    if (pos == std::string::npos)
        return "";

    return rest.substr(pos);
}

bool olc_room_handle_exit_keyword(CHAR_DATA* ch, EXIT_DATA* ex)
{
    bool succ = false;
    if (!ch || !ch->desc || !ch->desc->olc || !ex)
        return false;

    std::string arg = olc_room_exit_remaining_args(ch->desc->olc->last_cmd_arg);
    succ = set_str_field(ex->keyword, arg);
    if (succ)
        ch_printf(ch, "Exit %s keyword updated\n", exit_dir_label(ex->vdir));
    else
        ch_printf(ch, "Exit %s keyword failed to update\n", exit_dir_label(ex->vdir));

    return succ;
}

bool olc_room_handle_exit_desc(CHAR_DATA* ch, EXIT_DATA* ex)
{
    bool succ = false;
    if (!ch || !ch->desc || !ch->desc->olc || !ex)
        return false;

    std::string arg = olc_room_exit_remaining_args(ch->desc->olc->last_cmd_arg);
    succ = set_str_field(ex->description, arg);
    if (succ)
        ch_printf(ch, "Exit %s desc updated\n", exit_dir_label(ex->vdir));
    else
        ch_printf(ch, "Exit %s desc failed to update\n", exit_dir_label(ex->vdir));

    return succ;
}

bool olc_room_handle_bexit_flags(CHAR_DATA* ch, ROOM_INDEX_DATA* room, EXIT_DATA* ex, const std::string& flags)
{
    if (!ch || !room || !ex)
        return false;

    if (flags.empty())
    {
        send_to_char("No flags specified.\n", ch);

        std::string col = olc_format_exit_flags_list();
        if (!col.empty())
        {
            send_to_char("Valid flags:\n", ch);
            send_to_char(col, ch);
        }

        send_to_char("Usage: bexit <selector> flags <flag> [flag]...\n", ch);
        return false;
    }

    OlcPendingExitSideEffect* p = olc_room_get_or_create_pending_reverse_exit(ch, room, ex);
    if (!p)
    {
        send_to_char("Unable to queue reverse exit changes.\n", ch);
        return false;
    }

    int bits = p->final_exit_info;
    std::istringstream fss(flags);
    std::string word;

    while (fss >> word)
    {
        bool remove = false;

        if (!word.empty() && word[0] == '-')
        {
            remove = true;
            word = word.substr(1);
        }

        int bit = get_exflag((word));
        if (bit < 0)
        {
            send_to_char(("Unknown flag: " + word + "\n"), ch);

            std::string col = olc_format_exit_flags_list();
            if (!col.empty())
            {
                send_to_char("Valid flags:\n", ch);
                send_to_char(col, ch);
            }

            return false;
        }

        if (remove)
            REMOVE_BIT(bits, (1 << bit));
        else
            SET_BIT(bits, (1 << bit));
    }

    p->type = OlcExitSideEffectType::UPSERT_REVERSE;
    p->final_exit_info = bits;

    send_to_char("Reverse exit flags queued.\n", ch);
    ch->desc->olc->dirty = true;
    return true;
}

bool olc_room_handle_bexit_key(CHAR_DATA* ch, ROOM_INDEX_DATA* room, EXIT_DATA* ex, std::istringstream& iss)
{
    if (!ch || !room || !ex)
        return false;

    int key = 0;
    if (!(iss >> key))
    {
        send_to_char("Invalid key vnum.\n", ch);
        send_to_char("Usage: bexit <selector> key <vnum>\n", ch);
        return false;
    }

    OlcPendingExitSideEffect* p = olc_room_get_or_create_pending_reverse_exit(ch, room, ex);
    if (!p)
    {
        send_to_char("Unable to queue reverse exit changes.\n", ch);
        return false;
    }

    p->type = OlcExitSideEffectType::UPSERT_REVERSE;
    p->final_key = key;

    send_to_char("Reverse exit key queued.\n", ch);
    ch->desc->olc->dirty = true;
    return true;
}

bool olc_room_handle_bexit_keyword(CHAR_DATA* ch, ROOM_INDEX_DATA* room, EXIT_DATA* ex)
{
    if (!ch || !ch->desc || !ch->desc->olc || !room || !ex)
        return false;

    OlcPendingExitSideEffect* p = olc_room_get_or_create_pending_reverse_exit(ch, room, ex);
    if (!p)
    {
        send_to_char("Unable to queue reverse exit changes.\n", ch);
        return false;
    }

    std::string arg = olc_room_exit_remaining_args(ch->desc->olc->last_cmd_arg);

    p->type = OlcExitSideEffectType::UPSERT_REVERSE;
    p->final_keyword = arg;

    send_to_char("Reverse exit keyword queued.\n", ch);
    ch->desc->olc->dirty = true;
    return true;
}

bool olc_room_edit_exit_field_impl(CHAR_DATA* ch, ROOM_INDEX_DATA* room, bool reverse_default)
{
    if (!ch || !ch->desc || !ch->desc->olc || !room)
        return false;

    std::string arg = ch->desc->olc->last_cmd_arg;

    if (arg.empty())
    {
        olc_show_room_exit_help(ch);
        return false;
    }

    std::istringstream iss(arg);
    std::string sel_str, cmd;
    iss >> sel_str;

    if (sel_str.empty())
    {
        olc_show_room_exit_help(ch);
        return false;
    }

    if (sel_str[0] == '-')
    {
        std::string target = sel_str.substr(1);
        OlcExitSelector sel = olc_room_resolve_exit_selector(room, target);

        if (!sel.ex)
        {
            send_to_char("No such exit.\n", ch);
            olc_show_room_exit_help(ch);
            return false;
        }

        std::string opt;
        iss >> opt;
        bool two_way = reverse_default || (!opt.empty() && !str_prefix(opt, "two"));

        return olc_room_handle_exit_delete(ch, room, sel.ex, two_way);
    }

    iss >> cmd;

    if (cmd.empty())
    {
        olc_show_room_exit_help(ch);
        return false;
    }

    if (isdigit(static_cast<unsigned char>(cmd[0])))
    {
        bool force_new = false;
        std::string dir_str = sel_str;

        if (!dir_str.empty() && dir_str[0] == '+')
        {
            force_new = true;
            dir_str.erase(0, 1);
        }

        int dir = dir_lookup(dir_str);
        if (dir < 0)
        {
            send_to_char("Exit creation requires a direction.\n", ch);
            return false;
        }

        int vnum = atoi(cmd.c_str());

        OlcExitCreateOptions opts;
        if (!olc_room_parse_exit_create_options(ch, iss, reverse_default, opts))
            return false;

        return olc_room_handle_exit_create(
            ch,
            room,
            dir,
            vnum,
            opts.two_way,
            force_new,
            reverse_default,
            opts);
    }

    const char* resolved = olc_room_match_exit_command(cmd);
    if (!resolved)
    {
        if (olc_room_exit_command_is_ambiguous(cmd))
            olc_room_show_ambiguous_exit_command(ch, cmd);
        else
        {
            send_to_char("Unknown exit command.\n", ch);
            olc_show_room_exit_help(ch);
        }
        return false;
    }

    OlcExitSelector sel = olc_room_resolve_exit_selector(room, sel_str);
    if (!sel.ex)
    {
        send_to_char("No such exit.\n", ch);
        return false;
    }

    if (!str_cmp(resolved, "delete"))
    {
        std::string opt;
        iss >> opt;
        bool two_way = reverse_default || (!opt.empty() && !str_prefix(opt, "two"));
        return olc_room_handle_exit_delete(ch, room, sel.ex, two_way);
    }

    if (!str_cmp(resolved, "flags"))
    {
        std::string flags;
        getline(iss, flags);

        size_t pos = flags.find_first_not_of(' ');
        if (pos == std::string::npos)
            flags.clear();
        else
            flags.erase(0, pos);

        if (reverse_default)
            olc_room_handle_bexit_flags(ch, room, sel.ex, flags);

        return olc_room_handle_exit_flags(ch, sel.ex, flags);
    }

    if (!str_cmp(resolved, "key"))
    {
        if (reverse_default)
            olc_room_handle_bexit_key(ch, room, sel.ex, iss);

        return olc_room_handle_exit_key(ch, sel.ex, iss);
    }

    if (!str_cmp(resolved, "desc"))
        return olc_room_handle_exit_desc(ch, sel.ex);

    if (!str_cmp(resolved, "keyword"))
    {
        if (reverse_default)
            olc_room_handle_bexit_keyword(ch, room, sel.ex);

        return olc_room_handle_exit_keyword(ch, sel.ex);
    }

    send_to_char("Unknown exit command.\n", ch);
    olc_show_room_exit_help(ch);
    return false;
}

bool olc_room_edit_exit_field(CHAR_DATA* ch, ROOM_INDEX_DATA* room)
{
    return olc_room_edit_exit_field_impl(ch, room, false);
}

bool olc_room_edit_bexit_field(CHAR_DATA* ch, ROOM_INDEX_DATA* room)
{
    return olc_room_edit_exit_field_impl(ch, room, true);
}

bool olc_room_in_edit_mode(CHAR_DATA* ch)
{
    return ch
        && ch->desc
        && ch->desc->olc
        && ch->desc->olc->mode == OlcEditMode::ROOM_INLINE;
}

bool olc_room_commit_current_room(CHAR_DATA* ch)
{
    if (!ch || !ch->desc || !ch->desc->olc)
        return false;

    auto sess = ch->desc->olc;
    if (!sess->working_copy || !sess->original)
        return false;

    if (!sess->dirty && sess->pending_exit_side_effects.empty())
        return false;

    auto live = static_cast<ROOM_INDEX_DATA*>(sess->original);
    auto work = static_cast<ROOM_INDEX_DATA*>(sess->working_copy);

    olc_room_apply_changes(live, work);
    olc_room_relink_exits(live);

    for (const auto& p : sess->pending_exit_side_effects)
        olc_room_apply_pending_exit_side_effect(p);

    sess->pending_exit_side_effects.clear();
    sess->dirty = false;
    return true;
}

void olc_room_discard_current_room_working_copy(CHAR_DATA* ch)
{
    if (!ch || !ch->desc || !ch->desc->olc)
        return;

    auto sess = ch->desc->olc;

    if (sess->working_copy)
    {
        olc_room_free(static_cast<ROOM_INDEX_DATA*>(sess->working_copy));
        sess->working_copy = nullptr;
    }

    if (sess->original_clone)
    {
        olc_room_free(static_cast<ROOM_INDEX_DATA*>(sess->original_clone));
        sess->original_clone = nullptr;
    }

    sess->pending_exit_side_effects.clear();
    sess->last_cmd_arg.clear();
    sess->dirty = false;
}

void olc_room_edit_enter(CHAR_DATA* ch)
{
    if (!ch || !ch->desc || !ch->in_room)
        return;

    if (ch->desc->olc)
    {
        send_to_char("You are already editing something.\n", ch);
        return;
    }

    olc_start(ch, ch->in_room, get_room_schema(), &room_olc_ops);

    if (!ch->desc->olc)
        return;

    ch->desc->olc->mode = OlcEditMode::ROOM_INLINE;
    ch->desc->olc->anchor_room = ch->in_room;

    do_olcshow(ch, "");

    send_to_char("Room editing mode enabled.\n", ch);
    send_to_char("To exit, type stop save/abort, or just save/abort.\n", ch);    
    send_to_char("You can move normally, and 'look' and 'look (extradesc)' will display your working room, not the original room.  However, movement will not create rooms.\n", ch);    
    send_to_char("The room will show as empty of objects and characters, only showing the room itself.\n", ch);
}

void olc_room_edit_switch_to_room(CHAR_DATA* ch, ROOM_INDEX_DATA* room)
{
    if (!ch || !ch->desc || !ch->desc->olc || !room)
        return;

    auto sess = ch->desc->olc;
    if (sess->mode != OlcEditMode::ROOM_INLINE)
        return;

    /* Commit old room into live world, but do NOT save area file */
    olc_commit_current(ch);

    /* Tear down old current-room draft */
    olc_discard_current_working_copy(ch);

    /* Retarget same inline session to the new room */
    sess->original = room;
    sess->working_copy = sess->ops->clone(room);
    sess->original_clone = sess->ops->clone(room);
    sess->schema = get_room_schema();
    sess->ops = &room_olc_ops;
    sess->anchor_room = room;
    sess->mode = OlcEditMode::ROOM_INLINE;

    send_to_char("Now editing this room.\n", ch);
}

bool olc_room_is_field_name(const std::string& input)
{
    if (input.empty())
        return false;

    const auto* schema = get_room_schema();

    if (olc_find_field_fuzzy(schema, input))
        return true;

    if (olc_field_name_is_ambiguous(schema, input))
        return true;

    return false;
}

void olc_room_edit_help(CHAR_DATA* ch)
{
    olc_show(ch, "help", "");
    return;
    send_to_char("Room editing mode commands:\n", ch);
    send_to_char("  help or ?           Show this help\n", ch);
    send_to_char("  show [field]        Show current room draft / OLC view\n", ch);
    send_to_char("  look [target]       Preview current room draft\n", ch);
    send_to_char("  commit              Commit current draft to live room (no area save)\n", ch);
    send_to_char("  revert              Revert current room draft to pre-edit state\n", ch);
    send_to_char("  stop save           Commit current room, save area, leave edit mode\n", ch);
    send_to_char("  stop abort          Discard current room draft, leave edit mode\n", ch);
    send_to_char("  save/abort          Equivalent of stop save/abort", ch);
    send_to_char("\n", ch);
    send_to_char("Field editing shortcuts:\n", ch);
    send_to_char("  name <text>\n", ch);
    send_to_char("  description\n", ch);
    send_to_char("  extradesc <keyword>\n", ch);
    send_to_char("  flags <flags>\n", ch);
    send_to_char("  sector_type <type>\n", ch);
    send_to_char("  tunnel <value>\n", ch);
    send_to_char("  televnum <vnum>\n", ch);
    send_to_char("  teledelay <value>\n", ch);
    send_to_char("  exit ...\n", ch);
    send_to_char("  bexit ...\n", ch);
    send_to_char("\n", ch);
    send_to_char("Movement still works while in room edit mode.\n", ch);
    send_to_char("Direction commands always take priority over field prefixes.\n", ch);
}

bool olc_room_edit_interpret(CHAR_DATA* ch, const std::string& command, const std::string& argument)
{
    char buf[MSL];
    //log_printf("OLC INLINE: command='%s'", command);
    SPRINTF(buf, "%s", argument.c_str());

    if (!olc_room_in_edit_mode(ch) || command.empty())
        return false;

    if (olc_is_direction_alias(command))
        return false;

    if (olc_command_matches(command, "help") || !str_cmp(command, "?"))
    {
        if (argument.empty())
            olc_room_edit_help(ch);
        else
        {
            SPRINTF(buf, "%s %s", "help", argument.c_str());
            do_redit2(ch, buf);
        }
        return true;
    }

    if (olc_command_matches(command, "stop"))
    {
        std::string arg;
        one_argument(argument, arg);

        if (!str_cmp(arg, "save"))
        {
            olc_stop(ch, true);
            return true;
        }

        if (!str_cmp(arg, "abort"))
        {
            olc_stop(ch, false);
            return true;
        }

        send_to_char("Syntax: stop save|abort\n", ch);
        return true;
    }
    if (!str_cmp(command, "abort") || !str_cmp(command, "cancel"))
    {
        olc_stop(ch, false);
        return true;
    }

    if (!str_cmp(command, "save"))
    {
        olc_stop(ch, true);
        return true;
    }

    if (olc_command_matches(command, "revert"))
    {
        olc_room_edit_revert(ch);
        return true;
    }

    if (olc_command_matches(command, "commit"))
    {
        if (olc_commit_current(ch))
            send_to_char("Current changes committed.\n", ch);
        else
            send_to_char("No pending changes to commit.\n", ch);
        return true;
    }

    if (olc_command_matches(command, "show"))
    {
        do_olcshow(ch, buf);
        return true;
    }

    if (olc_command_matches(command, "olcshow"))
    {
        do_olcshow(ch, buf);
        return true;
    }

    if (olc_command_matches(command, "look"))
    {
        auto sess = ch->desc->olc;
        olc_show_room_preview(ch,
            static_cast<ROOM_INDEX_DATA*>(sess->working_copy),
            buf);
        return true;
    }

    if (olc_command_matches(command, "olcset"))
    {
        do_olcset(ch, buf);
        return true;
    }

    if (olc_room_is_field_name(command))
    {
        snprintf(buf, sizeof(buf), "%s %s", command.c_str(), argument.empty() ? "" : argument.c_str());
        do_olcset(ch, buf);
        return true;
    }

    return false;
}

// --------------------------------------------
// OLC_SHOW_ROOM
// -------------------ROOMS--------------------
// --------------------------------------------

void olc_show_room_preview_exits( CHAR_DATA* ch, ROOM_INDEX_DATA* room )
{
    char buf[MAX_STRING_LENGTH];    
    EXIT_DATA *pexit;    
    set_char_color( AT_EXITS, ch );
    bool found = false;

    ch_printf(ch,  "Exits:\n" );
    for ( pexit = room->first_exit; pexit; pexit = pexit->next )    
    {
        if ( pexit->to_room
        &&  !IS_SET(pexit->exit_info, EX_HIDDEN) )        
            found = true;
        buf[0] = '\0';        
        if ( IS_SET(pexit->exit_info, EX_CLOSED) )
        {
            STRAPP( buf, "%-5s - (closed)",
            capitalize( dir_name[pexit->vdir] ).c_str() );
        }
        else if ( IS_SET(pexit->exit_info, EX_WINDOW) )
        {
            STRAPP( buf, "%-5s - (window)",
            capitalize( dir_name[pexit->vdir] ).c_str() );
        }
        else if ( IS_SET(pexit->exit_info, EX_xAUTO) )
        {
        STRAPP( buf, "%-5s - %s",
            capitalize( pexit->keyword ).c_str(),
            room_is_dark( pexit->to_room )
            ?  "Too dark to tell"
            : pexit->to_room->name );
        }
        else
            STRAPP( buf, "%-5s - %s",
            capitalize( dir_name[pexit->vdir] ).c_str(),
            room_is_dark( pexit->to_room )
            ?  "Too dark to tell"
            : pexit->to_room->name ); 
        ch_printf(ch, "%s\n", buf);
    }   
    buf[0] = '\0';         
    if ( !found )
        ch_printf(ch, "%s", "none.\n");
}

void olc_show_room_exit_help(CHAR_DATA* ch)
{
    send_to_char("Usage:\n", ch);
    send_to_char("  (b)exit  <dir> <vnum> [two] [flags] [key] [keyword]\n", ch);
    send_to_char("  (b)exit  +<dir> <vnum> [two] [flags] [key] [keyword]\n", ch);
    send_to_char("  (b)exit -<selector>\n", ch);
    send_to_char("  (b)exit <selector> delete [two]\n", ch);
    send_to_char("  (b)exit <selector> flags <flag> [flag]...\n", ch);
    send_to_char("  (b)exit <selector> key <vnum>\n", ch);
    send_to_char("  (b)exit <selector> keyword <text>\n", ch);
    send_to_char("  exit <selector> desc <text> - Does not allow bexit\n", ch);
    send_to_char("  Using bexit instead of exit is two-way by default\n", ch);
    send_to_char("  Create extras must be given in order: [flags] [key] [keyword]\n", ch);
    send_to_char("  You may omit trailing values, but not a middle value.\n", ch);
    send_to_char("Selectors:\n", ch);
    send_to_char("  east         first exit east\n", ch);
    send_to_char("  2.east       second exit east\n", ch);
    send_to_char("  #3           third exit in room's exit list\n", ch);
    send_to_char("  ?            first somewhere exit\n", ch);
    send_to_char("  2.?          second somewhere exit\n", ch);
    send_to_char("  <keyword>    first exit matching keyword\n", ch);

    std::string col = olc_format_exit_flags_list();
    if (!col.empty())
    {
        send_to_char("Valid flags:\n", ch);
        send_to_char(col, ch);
    }
}

void olc_show_room_preview(CHAR_DATA* ch, ROOM_INDEX_DATA* room, const char* argument)
{
    if (!ch || !room)
        return;

    std::string arg = argument ? argument : "";
    std::string out;    
    if (arg.empty())
    {
        arg.erase(0, arg.find_first_not_of(' '));

        set_char_color( AT_RMNAME, ch);
        send_to_char( room->name, ch);
        send_to_char(" ", ch);

        if( !ch->desc->original )
        {
            if( ( get_trust( ch ) >= LEVEL_IMMORTAL ) )             
            {
                if( BV_IS_SET( ch->act, PLR_ROOMVNUM ) )
                {
                    set_char_color( AT_BLUE, ch );      /* Added 10/17 by Kuran of */
                    send_to_char( "{", ch );            /* SWReality */
                    ch_printf( ch, "%d", room->vnum );
                    send_to_char( "}", ch );
                }
                if( BV_IS_SET( ch->pcdata->flags, PCFLAG_ROOM ) )
                {
                    set_char_color( AT_CYAN, ch );
                    send_to_char( "[", ch );
                    send_to_char( bitset_to_string( room->room_flags, r_flags ), ch );
                    send_to_char( "]", ch );
                }
            }
        }

        send_to_char( "\n", ch );
        set_char_color( AT_RMDESC, ch ); 	
        
        if ( ( !IS_NPC(ch) && !BV_IS_SET(ch->act, PLR_BRIEF) ) )
            send_to_char( room->description, ch );

        if ( BV_IS_SET( room->room_flags, ROOM_CAN_LAND ) )
            send_to_char( "&GYou can rent ships here. ( Type rent )&R&W\n", ch );
        
        if ( is_bus_stop( room->vnum ) )
            send_to_char( "&GA Serin shuttle picks up passengers here. ( Type findserin )&R&W\n", ch );
            
        if ( !IS_NPC(ch) && BV_IS_SET(ch->act, PLR_AUTOEXIT) )
                olc_show_room_preview_exits( ch, room );
    }
    const char* ed = get_extra_descr(arg, room->first_extradesc);
    if (ed)
    {
        out = ed;
        if (!out.empty())
        {
            send_to_char("\n", ch);
            send_to_char(out, ch);
            return;
        }
    }
}

void olc_show_room(CHAR_DATA* ch, ROOM_INDEX_DATA* room, bool show_help, int term_width)
{
    if (!ch || !room)
        return;

    char buf[MSL];

    const char* area_name = (room->area && room->area->name)
        ? room->area->name : "unknown";

    const char* filename = (room->area && room->area->filename)
        ? room->area->filename : "unknown";

    snprintf(buf, sizeof(buf),
        "%s[Room %d]%s %s%s%s (%s%s%s) Lit: %s%s (%d)%s\n",
        OLC_COL_HEADER,
        room->vnum,
        OLC_COL_RESET,
        OLC_COL_STRING,
        area_name,
        OLC_COL_RESET,
        OLC_COL_STRING,
        filename,
        OLC_COL_RESET,
        OLC_COL_INT,
        (room->light > 0) ? "Yes" : "No",
        room->light,
        OLC_COL_RESET
    );
    send_to_char(buf, ch);

    std::vector<std::string> other_lines;
    const int label_width = 11;

    {
        std::string room_name =
            (room->name && room->name[0]) ? room->name : "(none)";

        auto lines = olc_format_line(
            "name",
            room_name,
            OLC_COL_STRING,
            label_width,
            term_width
        );

        for (const auto& l : lines)
            other_lines.push_back(l);
    }

    {
        std::string desc =
            (room->description && room->description[0]) ? room->description : "(none)";

        auto lines = olc_format_line(
            "description",
            desc,
            OLC_COL_STRING,
            label_width,
            term_width
        );

        for (const auto& l : lines)
            other_lines.push_back(l);
    }

    {
        std::string sector =
            (room->sector_type >= 0)
                ? get_flag_name(sect_types, room->sector_type, SECT_MAX)
                : std::to_string(room->sector_type);

        auto lines = olc_format_line(
            "sector_type",
            sector,
            OLC_COL_ENUM,
            label_width,
            term_width
        );

        for (const auto& l : lines)
            other_lines.push_back(l);
    }

    {
        std::string flags = bitset_to_string(room->room_flags, r_flags);
        if (flags.empty())
            flags = "none";

        auto lines = olc_format_line(
            "flags",
            flags,
            OLC_COL_LIST,
            label_width,
            term_width
        );

        for (const auto& l : lines)
            other_lines.push_back(l);
    }

    {
        auto lines = olc_format_line(
            "tunnel",
            std::to_string(room->tunnel),
            OLC_COL_INT,
            label_width,
            term_width
        );

        for (const auto& l : lines)
            other_lines.push_back(l);
    }

    {
        auto lines = olc_format_line(
            "televnum",
            std::to_string(room->tele_vnum),
            OLC_COL_INT,
            label_width,
            term_width
        );

        for (const auto& l : lines)
            other_lines.push_back(l);
    }

    {
        auto lines = olc_format_line(
            "teledelay",
            std::to_string(room->tele_delay),
            OLC_COL_INT,
            label_width,
            term_width
        );

        for (const auto& l : lines)
            other_lines.push_back(l);
    }

    {
        std::string summary = olc_room_extradesc_list_summary(room);
        auto lines = olc_format_line(
            "extradesc",
            summary.empty() ? "none" : summary,
            OLC_COL_LIST,
            label_width,
            term_width
        );

        for (const auto& l : lines)
            other_lines.push_back(l);
    }

    {
        auto lines = olc_format_exit_list_lines(
            room,
            label_width,
            term_width,
            show_help
        );

        for (const auto& l : lines)
            other_lines.push_back(l);
    }

    {
        auto pending_lines = olc_format_pending_exit_side_effects(
            ch,
            label_width,
            term_width
        );

        for (const auto& l : pending_lines)
            other_lines.push_back(l);
    }

    for (const auto& l : other_lines)
        send_to_char((l + "\n"), ch);
}

// --------------------------------------------
// OBJECT_OLC object_olc OlcOps functions
// --------------------------------------------
void* object_olc_clone(const void* src)
{
    return olc_object_clone(static_cast<const OBJ_DATA*>(src));
}

void object_olc_free_clone(void* obj)
{
    olc_object_free_clone(static_cast<OBJ_DATA*>(obj));
}

void object_olc_apply_changes(void* original, void* working)
{
    OBJ_DATA* live = static_cast<OBJ_DATA*>(original);
    OBJ_DATA* draft = static_cast<OBJ_DATA*>(working);

    if (!live || !draft)
        return;

    olc_object_apply_instance_changes(live, draft);
}


void object_olc_save(CHAR_DATA* ch, void* original)
{
    if (!ch || !original)
        return;

    OBJ_DATA* obj = static_cast<OBJ_DATA*>(original);

    if (!obj->pIndexData)
    {
        send_to_char("No prototype associated.\n", ch);
        return;
    }

    if (!IS_OBJ_STAT(obj, ITEM_PROTOTYPE))
    {
        send_to_char("This object is not marked ITEM_PROTOTYPE; only the live instance was changed.\n", ch);
        return;
    }

 //   AREA_DATA* area = obj->pIndexData->area;
//    fold_area(area, area->filename, FALSE);
//    send_to_char("Area saved.\n", ch);
}

void object_olc_after_commit(CHAR_DATA* ch, void* original, void* working)
{
    if (!ch || !ch->desc || !ch->desc->olc)
        return;

    auto sess = ch->desc->olc;
    OBJ_DATA* live = static_cast<OBJ_DATA*>(original);
    OBJ_DATA* draft = static_cast<OBJ_DATA*>(working);
    OBJ_DATA* baseline = static_cast<OBJ_DATA*>(sess->original_clone);

    if (!live || !draft || !baseline)
        return;

    if (IS_OBJ_STAT(live, ITEM_PROTOTYPE) && live->pIndexData)
    {
        olc_object_apply_prototype_changes(live->pIndexData, draft, baseline);
    }
}

void object_olc_after_revert(CHAR_DATA* ch, void* original, void* working)
{
    /* nothing needed yet */
}


const OlcOps object_olc_ops =
{
    object_olc_clone,
    object_olc_free_clone,
    object_olc_apply_changes,
    object_olc_save,
    object_olc_after_commit,
    object_olc_after_revert,
    olc_object_edit_interpret,
    OlcInterpretStage::EARLY    
};

// --------------------------------------------
// OLC_OBJECT olc_object specific functions
// --------------------------------------------

AFFECT_DATA*& olc_object_first_affect_ref(void* obj)
{
    return static_cast<OBJ_DATA*>(obj)->first_affect;
}

AFFECT_DATA*& olc_object_last_affect_ref(void* obj)
{
    return static_cast<OBJ_DATA*>(obj)->last_affect;
}

AFFECT_DATA* olc_object_find_affect_by_number_wrap(void* obj, int index)
{
    return olc_find_object_affect_by_number(static_cast<OBJ_DATA*>(obj), index);
}

bool olc_object_parse_affect_value_wrap(CHAR_DATA* ch, int loc, const std::string& value_text, int& out_value)
{
    return olc_object_parse_affect_value(ch, loc, value_text, out_value);
}

std::vector<std::string> olc_object_format_single_affect_lines(
    AFFECT_DATA* paf,
    size_t max_name_len,
    int term_width)
{
    std::vector<std::string> out;

    if (!paf)
        return out;

    char locbuf[MSL];
    char modbuf[MSL];

    snprintf(locbuf, sizeof(locbuf), "%d", paf->location);
    snprintf(modbuf, sizeof(modbuf), "%d", paf->modifier);

    olc_append_lines(out, olc_format_line("location", locbuf, OLC_COL_ENUM, (int)max_name_len, term_width));
    olc_append_lines(out, olc_format_line("modifier", modbuf, OLC_COL_INT, (int)max_name_len, term_width));

    return out;
}

bool olc_object_edit_revert(CHAR_DATA* ch)
{
    if (!olc_object_in_edit_mode(ch))
        return false;

    auto sess = ch->desc->olc;

    if (!sess->original_clone || !sess->ops || !sess->ops->clone || !sess->ops->free_clone)
    {
        send_to_char("No revert snapshot is available.\n", ch);
        return false;
    }

    if (sess->working_copy)
        sess->ops->free_clone(sess->working_copy);

    sess->working_copy = sess->ops->clone(sess->original_clone);
    if (!sess->working_copy)
    {
        send_to_char("Failed to restore revert snapshot.\n", ch);
        return false;
    }

    sess->last_cmd_arg.clear();
    sess->dirty = false;

    if (sess->ops->after_revert)
        sess->ops->after_revert(ch, sess->original, sess->working_copy);

    send_to_char("Object reverted to pre-edit state.\n", ch);
    return true;
}

OBJ_DATA* olc_object_clone(const OBJ_DATA* src)
{
    if (!src)
        return nullptr;

    OBJ_DATA* dst = clone_object(const_cast<OBJ_DATA*>(src));
    if (!dst)
        return nullptr;

    if (dst->next || dst->prev || dst == first_object || dst == last_object)
        UNLINK(dst, first_object, last_object, next, prev);

    /* Detach world/runtime linkage */
    dst->next = nullptr;
    dst->prev = nullptr;
    dst->next_content = nullptr;
    dst->prev_content = nullptr;
    dst->first_content = nullptr;
    dst->last_content = nullptr;
    dst->in_obj = nullptr;
    dst->carried_by = nullptr;
    dst->in_room = nullptr;

    /* Runtime-only / not edited */
    if (dst->armed_by)
    {
        STRFREE(dst->armed_by);
        dst->armed_by = nullptr;
    }

    dst->mpact = nullptr;
    dst->mpactnum = 0;
    dst->mpscriptpos = 0;
    dst->blaster_setting = 0;

    set_str_field(dst->name,        src->name        ? src->name        : "");
    set_str_field(dst->short_descr, src->short_descr ? src->short_descr : "");
    set_str_field(dst->description, src->description ? src->description : "");
    set_str_field(dst->action_desc, src->action_desc ? src->action_desc : "");

    dst->item_type   = src->item_type;
    dst->trig_flags  = src->trig_flags;
    dst->wear_flags  = src->wear_flags;
    dst->wear_loc    = src->wear_loc;
    dst->weight      = src->weight;
    dst->cost        = src->cost;
    dst->level       = src->level;
    dst->timer       = src->timer;
    dst->objflags    = src->objflags;
    dst->count       = src->count;

    for (int i = 0; i < 6; ++i)
        dst->value[i] = src->value[i];

    /* Rebuild extra descs from the source object */
    if (dst->first_extradesc)
        olc_room_free_extra_descs(dst->first_extradesc);

    dst->first_extradesc = olc_room_clone_extra_descs(src->first_extradesc);
    dst->last_extradesc = nullptr;
    for (auto ed = dst->first_extradesc; ed; ed = ed->next)
        dst->last_extradesc = ed;

    /* Rebuild affects from the source object */
    if (dst->first_affect)
        olc_free_affects(dst->first_affect);

    dst->first_affect = olc_clone_affects(src->first_affect);
    dst->last_affect = nullptr;
    for (auto af = dst->first_affect; af; af = af->next)
        dst->last_affect = af;

    return dst;
}

void olc_object_free_clone(OBJ_DATA* obj)
{
    if (!obj)
        return;

    if (obj->name)
        STRFREE(obj->name);

    if (obj->short_descr)
        STRFREE(obj->short_descr);

    if (obj->description)
        STRFREE(obj->description);

    if (obj->action_desc)
        STRFREE(obj->action_desc);

    if (obj->armed_by)
        STRFREE(obj->armed_by);

    if (obj->first_extradesc)
        olc_room_free_extra_descs(obj->first_extradesc);

    if (obj->first_affect)
        olc_free_affects(obj->first_affect);

    obj->first_extradesc = nullptr;
    obj->last_extradesc = nullptr;
    obj->first_affect = nullptr;
    obj->last_affect = nullptr;

    /* Undo clone_object() bookkeeping */
    if (obj->pIndexData)
        obj->pIndexData->count -= obj->count;

    numobjsloaded -= obj->count;
    --physicalobjects;

    delete obj;
}

void olc_object_apply_instance_changes(OBJ_DATA* dst, OBJ_DATA* src)
{
    if (!dst || !src)
        return;

    /* strings */
    set_str_field(dst->name, src->name ? src->name : "");
    set_str_field(dst->short_descr, src->short_descr ? src->short_descr : "");
    set_str_field(dst->description, src->description ? src->description : "");
    set_str_field(dst->action_desc, src->action_desc ? src->action_desc : "");

    /* scalars / flags */
    dst->item_type = src->item_type;
    dst->wear_flags = src->wear_flags;
    dst->wear_loc = src->wear_loc;
    dst->weight = src->weight;
    dst->cost = src->cost;
    dst->level = src->level;
    dst->timer = src->timer;
    dst->objflags = src->objflags;
    dst->trig_flags = src->trig_flags;
    dst->count = src->count;

    for (int i = 0; i < 6; ++i)
        dst->value[i] = src->value[i];

    /* extra descs */
    if (dst->first_extradesc)
        olc_room_free_extra_descs(dst->first_extradesc);

    dst->first_extradesc = olc_room_clone_extra_descs(src->first_extradesc);
    dst->last_extradesc = nullptr;
    for (auto ed = dst->first_extradesc; ed; ed = ed->next)
        dst->last_extradesc = ed;

    /* affects */
    if (dst->first_affect)
        olc_free_affects(dst->first_affect);

    dst->first_affect = olc_clone_affects(src->first_affect);
    dst->last_affect = nullptr;
    for (auto af = dst->first_affect; af; af = af->next)
        dst->last_affect = af;
}

void olc_object_apply_prototype_changes(
    OBJ_INDEX_DATA* dst,
    const OBJ_DATA* edited,
    const OBJ_DATA* baseline)
{
    if (!dst || !edited || !baseline)
        return;

    /* Strings */
    if (!olc_str_eq(edited->name, baseline->name))
        set_str_field(dst->name, edited->name ? edited->name : "");

    if (!olc_str_eq(edited->short_descr, baseline->short_descr))
        set_str_field(dst->short_descr, edited->short_descr ? edited->short_descr : "");

    if (!olc_str_eq(edited->description, baseline->description))
        set_str_field(dst->description, edited->description ? edited->description : "");

    if (!olc_str_eq(edited->action_desc, baseline->action_desc))
        set_str_field(dst->action_desc, edited->action_desc ? edited->action_desc : "");

    /* Scalars / enums / flags that belong on the prototype */
    if (edited->item_type != baseline->item_type)
        dst->item_type = edited->item_type;

    if (!(edited->wear_flags == baseline->wear_flags))
        dst->wear_flags = edited->wear_flags;

    if (!(edited->trig_flags == baseline->trig_flags))
        dst->trig_flags = edited->trig_flags;

    if (edited->weight != baseline->weight)
        dst->weight = edited->weight;

    if (edited->cost != baseline->cost)
        dst->cost = edited->cost;

    if (edited->level != baseline->level)
        dst->level = edited->level;

    /* objflags are applied as a delta relative to the original clone.
       This avoids overwriting prototype-only differences that already existed. */
    {
        olc_apply_flag_delta(dst->objflags, edited->objflags, baseline->objflags);
    }

    if (edited->count != baseline->count)
        dst->count = edited->count;

    /* value[0..5] */
    for (int i = 0; i < 6; ++i)
    {
        if (edited->value[i] != baseline->value[i])
            dst->value[i] = edited->value[i];
    }

    /* Extra descriptions */
    if (!olc_extradescs_equal(edited->first_extradesc, baseline->first_extradesc))
    {
        if (dst->first_extradesc)
            olc_room_free_extra_descs(dst->first_extradesc);

        dst->first_extradesc = olc_room_clone_extra_descs(edited->first_extradesc);
        dst->last_extradesc = nullptr;
        for (auto ed = dst->first_extradesc; ed; ed = ed->next)
            dst->last_extradesc = ed;
    }

    /* Affects */
    if (!olc_affects_equal(edited->first_affect, baseline->first_affect))
    {
        if (dst->first_affect)
            olc_free_affects(dst->first_affect);

        dst->first_affect = olc_clone_affects(edited->first_affect);
        dst->last_affect = nullptr;
        for (auto af = dst->first_affect; af; af = af->next)
            dst->last_affect = af;
    }
}

void olc_object_edit_enter(CHAR_DATA* ch, OBJ_DATA* obj)
{
    if (!ch || !ch->desc || !obj)
        return;

    if (ch->desc->olc)
    {
        send_to_char("You are already editing something.\n", ch);
        return;
    }

    olc_start(ch, obj, get_object_schema(), &object_olc_ops);

    if (!ch->desc->olc)
        return;

    send_to_char("Object editing mode enabled.\n", ch);
}

void olc_object_show_header(CHAR_DATA* ch, OBJ_DATA* obj)
{
    if (!ch || !obj)
        return;

    ch_printf(ch,
        "%sObject%s: %s%s%s  %s[Vnum %d]%s  %s[Serial %d]%s\n",
        OLC_COL_HEADER, OLC_COL_RESET,
        OLC_COL_STRING, obj->short_descr ? obj->short_descr : "(no short)", OLC_COL_RESET,
        OLC_COL_INT, obj->pIndexData ? obj->pIndexData->vnum : 0, OLC_COL_RESET,
        OLC_COL_INT, obj->serial, OLC_COL_RESET
    );
}

EXTRA_DESCR_DATA* olc_object_find_extra_desc(OBJ_DATA* obj, const std::string& keyword)
{
    if (!obj || keyword.empty())
        return nullptr;

    EXTRA_DESCR_DATA* best_match = nullptr;
    MatchResult best_result;

    for (auto ed = obj->first_extradesc; ed; ed = ed->next)
    {
        if (!ed->keyword)
            continue;

        MatchResult result = match_keywords(ed->keyword, keyword);

        if (result.type == MatchType::EXACT)
            return ed;

        if (result.type == MatchType::PREFIX &&
            result.match_length > best_result.match_length)
        {
            best_result = result;
            best_match = ed;
        }
    }

    return best_match;
}

EXTRA_DESCR_DATA* olc_object_get_or_create_extra_desc(OBJ_DATA* obj, const std::string& keyword)
{
    auto ed = olc_object_find_extra_desc(obj, keyword);
    if (ed)
        return ed;

    ed = new EXTRA_DESCR_DATA{};
    ed->keyword = STRALLOC(keyword);
    ed->description = STRALLOC("");

    ed->prev = nullptr;
    ed->next = obj->first_extradesc;

    if (obj->first_extradesc)
        obj->first_extradesc->prev = ed;
    else
        obj->last_extradesc = ed;

    obj->first_extradesc = ed;
    return ed;
}

bool olc_object_remove_extra_desc(OBJ_DATA* obj, const char* keyword)
{
    if (!obj || !keyword || keyword[0] == '\0')
        return false;

    for (auto ed = obj->first_extradesc; ed; ed = ed->next)
    {
        if (is_name(keyword, ed->keyword))
        {
            if (ed->prev)
                ed->prev->next = ed->next;
            else
                obj->first_extradesc = ed->next;

            if (ed->next)
                ed->next->prev = ed->prev;
            else
                obj->last_extradesc = ed->prev;

            if (ed->keyword)
                STRFREE(ed->keyword);
            if (ed->description)
                STRFREE(ed->description);

            delete ed;
            return true;
        }
    }

    return false;
}

std::string olc_object_extradesc_list_summary(OBJ_DATA* obj)
{
    if (!obj)
        return "none";

    std::string out;

    for (auto ed = obj->first_extradesc; ed; ed = ed->next)
    {
        if (ed->keyword && ed->keyword[0] != '\0')
        {
            if (!out.empty())
                out += " ";
            out += ed->keyword;
        }
    }

    return out.empty() ? "none" : out;
}

bool olc_object_finish_extradesc_edit(
    CHAR_DATA* ch, OBJ_DATA* obj, const std::string& value )
{
    EXTRA_DESCR_DATA *ed = nullptr;

    (void)obj;

    if ( !ch || !ch->desc || !ch->desc->olc || !ch->desc->olc->editor_context )
    {
        bug( "olc_object_finish_extradesc_edit: NULL editor_context", 0 );
        return false;
    }

    ed = static_cast<EXTRA_DESCR_DATA*>( ch->desc->olc->editor_context );
    ch->desc->olc->editor_context = nullptr;

    return set_str_field( ed->description, value );
}

bool olc_object_edit_extradesc_field(CHAR_DATA* ch, OBJ_DATA* obj)
{
    const char* arg = ch->desc->olc->last_cmd_arg.c_str();

    if (!arg || arg[0] == '\0')
    {
        olc_show_object_extradesc_help(ch);
        return false;
    }

    if (arg[0] == '-')
    {
        const char* key = arg + 1;

        if (!key || key[0] == '\0')
        {
            send_to_char("Delete which keyword?\n", ch);
            return false;
        }

        if (olc_object_remove_extra_desc(obj, key))
        {
            send_to_char("Extra description deleted.\n", ch);
            ch->desc->olc->dirty = true;
        }
        else
        {
            send_to_char("No such extra description.\n", ch);
        }

        return true;
    }

    auto ed = olc_object_get_or_create_extra_desc(obj, arg);

    ch->substate = SUB_OBJ_EXTRA;
    ch->dest_buf = obj;      
    ch->desc->olc->editor_context = ed;
    ch->last_cmd = do_olcset;
    start_editing(ch, ed->description);
    return true;
}

AFFECT_DATA* olc_find_object_affect_by_number(OBJ_DATA* obj, int index)
{
    if (!obj || index <= 0)
        return nullptr;

    int count = 0;
    for (AFFECT_DATA* af = obj->first_affect; af; af = af->next)
    {
        if (++count == index)
            return af;
    }

    return nullptr;
}

std::string olc_object_affect_list_summary(OBJ_DATA* obj)
{
    if (!obj || !obj->first_affect)
        return "none";

    std::string out;
    int count = 0;

    for (AFFECT_DATA* af = obj->first_affect; af; af = af->next)
    {
        ++count;

        if (!out.empty())
            out += "  ";

        out += "#";
        out += std::to_string(count);
        out += "=";

        if (af->location >= 0 && af->location < MAX_APPLY_TYPE)
            out += get_flag_name(a_types, af->location, MAX_APPLY_TYPE);
        else
            out += "?";
    }

    return out.empty() ? "none" : out;
}

std::string olc_object_affect_location_name(int loc)
{
    if (loc >= 0 && loc < MAX_APPLY_TYPE)
        return a_types[loc].name;

    return std::to_string(loc);
}

std::vector<std::string> olc_object_format_affect_lines(
    OBJ_DATA* obj,
    size_t label_width,
    int term_width)
{
    std::vector<std::string> out;

    if (!obj || !obj->first_affect)
    {
        char buf[MSL];
        snprintf(buf, sizeof(buf),
            "%s%-*s%s : %snone%s",
            OLC_COL_LABEL,
            (int)label_width,
            "Affects",
            OLC_COL_RESET,
            OLC_COL_LIST,
            OLC_COL_RESET
        );
        out.push_back(buf);
        return out;
    }

    bool first_line = true;
    int count = 0;

    for (AFFECT_DATA* af = obj->first_affect; af; af = af->next)
    {
        ++count;

        std::string num = "#" + std::to_string(count) + ":";
        std::string loc = olc_object_affect_location_name(af->location);

        std::string tail =
            "Modifier: " + std::to_string(af->modifier) +
            "  Duration: " + std::to_string(af->duration) +
            "  Type: " + std::to_string(af->type) +
            "  Bitvector: " + (af->bitvector >= 0 ? get_flag_name(aff_flags,af->bitvector, AFF_MAX) : "none");

        char prefix[MSL];
        snprintf(prefix, sizeof(prefix),
            "%s%-*s%s : %s%-4s%s %s%-16s%s  ",
            OLC_COL_LABEL,
            (int)label_width,
            first_line ? "Affects" : "",
            OLC_COL_RESET,
            OLC_COL_INT,
            num.c_str(),
            OLC_COL_RESET,
            OLC_COL_ENUM,
            loc.c_str(),
            OLC_COL_RESET
        );

        int prefix_len = visible_length(prefix);
        int available = term_width - prefix_len;
        if (available < 20)
            available = 20;

        auto wrapped = olc_format_wrap_text(tail, available, 0);
        if (wrapped.empty())
            wrapped.push_back(tail);

        out.push_back(std::string(prefix) + OLC_COL_STRING + wrapped[0] + OLC_COL_RESET);

        for (size_t i = 1; i < wrapped.size(); ++i)
        {
            std::string indent(prefix_len, ' ');
            char cont[MSL];
            snprintf(cont, sizeof(cont),
                "%s%s%s%s",
                indent.c_str(),
                OLC_COL_STRING,
                wrapped[i].c_str(),
                OLC_COL_RESET
            );
            out.push_back(cont);
        }

        first_line = false;
    }

    return out;
}

bool olc_object_parse_affect_value(
    CHAR_DATA* ch,
    int loc,
    const std::string& value_text,
    int& out_value)
{
    if (loc == APPLY_AFFECT)
    {
        out_value = get_aflag(value_text);
        if (out_value < 0)
        {
            ch_printf(ch, "Unknown affect flag: %s\n", value_text.c_str());
            return false;
        }
        return true;
    }

    if (loc > APPLY_AFFECT && loc < APPLY_WEAPONSPELL)
    {
        out_value = get_risflag((value_text));
        if (out_value < 0)
        {
            ch_printf(ch, "Unknown resistance flag: %s\n", value_text.c_str());
            return false;
        }
        return true;
    }

    return olc_format_parse_strict_int(value_text, out_value);
}

bool olc_object_values_equal_display(const OBJ_DATA* obj, const OBJ_INDEX_DATA* proto)
{
    if (!obj || !proto)
        return false;

    for (int i = 0; i < 6; ++i)
    {
        if (obj->value[i] != proto->value[i])
            return false;
    }

    return true;
}

bool olc_object_extradescs_equal_proto(const OBJ_DATA* obj, const OBJ_INDEX_DATA* proto)
{
    if (!obj || !proto)
        return false;

    return olc_extradescs_equal(obj->first_extradesc, proto->first_extradesc);
}

std::string olc_object_proto_extradesc_list_summary(const OBJ_INDEX_DATA* obj)
{
    if (!obj)
        return "none";

    std::string out;

    for (auto ed = obj->first_extradesc; ed; ed = ed->next)
    {
        if (ed->keyword && ed->keyword[0] != '\0')
        {
            if (!out.empty())
                out += " ";
            out += ed->keyword;
        }
    }

    return out.empty() ? "none" : out;
}

std::vector<std::string> olc_object_format_proto_affect_lines(
    OBJ_INDEX_DATA* obj,
    size_t label_width,
    int term_width)
{
    std::vector<std::string> out;

    if (!obj || !obj->first_affect)
    {
        char buf[MSL];
        snprintf(buf, sizeof(buf),
            "%s%-*s%s : %snone%s",
            OLC_COL_LABEL,
            (int)label_width,
            "Prototype",
            OLC_COL_RESET,
            OLC_COL_PROTO_DIFF,
            OLC_COL_RESET
        );
        out.push_back(buf);
        return out;
    }

    bool first_line = true;
    int count = 0;

    for (AFFECT_DATA* af = obj->first_affect; af; af = af->next)
    {
        ++count;

        std::string num = "#" + std::to_string(count) + ":";
        std::string loc = olc_object_affect_location_name(af->location);

        std::string tail =
            "Modifier: " + std::to_string(af->modifier) +
            "  Duration: " + std::to_string(af->duration) +
            "  Type: " + std::to_string(af->type) +
            "  Bitvector: " + (af->bitvector >= 0 ? get_flag_name(aff_flags,af->bitvector, AFF_MAX) : "none");

        char prefix[MSL];
        snprintf(prefix, sizeof(prefix),
            "%s%-*s%s : %s%-4s%s %s%-16s%s  ",
            OLC_COL_LABEL,
            (int)label_width,
            first_line ? "Prototype" : "",
            OLC_COL_RESET,
            OLC_COL_INT,
            num.c_str(),
            OLC_COL_RESET,
            OLC_COL_PROTO_DIFF,
            loc.c_str(),
            OLC_COL_RESET
        );

        int prefix_len = visible_length(prefix);
        int available = term_width - prefix_len;
        if (available < 20)
            available = 20;

        auto wrapped = olc_format_wrap_text(tail, available, 0);
        if (wrapped.empty())
            wrapped.push_back(tail);

        out.push_back(std::string(prefix) + OLC_COL_PROTO_DIFF + wrapped[0] + OLC_COL_RESET);

        for (size_t i = 1; i < wrapped.size(); ++i)
        {
            std::string indent(prefix_len, ' ');
            char cont[MSL];
            snprintf(cont, sizeof(cont),
                "%s%s%s%s",
                indent.c_str(),
                OLC_COL_PROTO_DIFF,
                wrapped[i].c_str(),
                OLC_COL_RESET
            );
            out.push_back(cont);
        }

        first_line = false;
    }

    return out;
}

std::string olc_object_values_summary(const OBJ_DATA* obj)
{
    char buf[MSL];

    if (!obj)
        return "none";

    snprintf(buf, sizeof(buf), "[%d] [%d] [%d] [%d] [%d] [%d]",
        obj->value[0], obj->value[1], obj->value[2],
        obj->value[3], obj->value[4], obj->value[5]);

    return buf;
}

std::string olc_object_proto_values_summary(const OBJ_INDEX_DATA* obj)
{
    char buf[MSL];

    if (!obj)
        return "none";

    snprintf(buf, sizeof(buf), "[%d] [%d] [%d] [%d] [%d] [%d]",
        obj->value[0], obj->value[1], obj->value[2],
        obj->value[3], obj->value[4], obj->value[5]);

    return buf;
}

const OlcItemValueInfo* olc_object_find_value_info(int item_type)
{
    extern const OlcItemValueInfo g_olc_item_value_info[];

    for (int i = 0; g_olc_item_value_info[i].item_type != -1; ++i)
    {
        if (g_olc_item_value_info[i].item_type == item_type)
            return &g_olc_item_value_info[i];
    }

    return nullptr;
}

std::vector<std::string> olc_object_labeled_value_pairs(const OBJ_DATA* obj)
{
    std::vector<std::string> out;

    if (!obj)
    {
        out.push_back("none");
        return out;
    }

    const OlcItemValueInfo* info = olc_object_find_value_info(obj->item_type);
    if (!info)
    {
        out.push_back("[no metadata]");
        return out;
    }

    if (!info->used)
    {
        out.push_back("[item type not used in codebase]");
        return out;
    }

    if (!info->values_used)
    {
        out.push_back("[values not used for this item type]");
        return out;
    }

    for (int i = 0; i < 6; ++i)
    {
        const char* label = info->labels[i];
        if (!label || label[0] == '\0')
            continue;

        out.push_back(std::string(label) + "=" + std::to_string(obj->value[i]));
    }

    if (out.empty())
        out.push_back("[values defined but no labels]");

    return out;
}

std::vector<std::string> olc_object_proto_labeled_value_pairs(const OBJ_INDEX_DATA* obj)
{
    std::vector<std::string> out;

    if (!obj)
    {
        out.push_back("none");
        return out;
    }

    const OlcItemValueInfo* info = olc_object_find_value_info(obj->item_type);
    if (!info)
    {
        out.push_back("[no metadata]");
        return out;
    }

    if (!info->used)
    {
        out.push_back("[item type not used in codebase]");
        return out;
    }

    if (!info->values_used)
    {
        out.push_back("[values not used for this item type]");
        return out;
    }

    for (int i = 0; i < 6; ++i)
    {
        const char* label = info->labels[i];
        if (!label || label[0] == '\0')
            continue;

        out.push_back(std::string(label) + "=" + std::to_string(obj->value[i]));
    }

    if (out.empty())
        out.push_back("[values defined but no labels]");

    return out;
}

bool olc_edit_object_affect_field(CHAR_DATA* ch, OBJ_DATA* obj)
{
    return olc_edit_affect_field_generic(
        ch,
        obj,
        olc_object_find_affect_by_number_wrap,
        olc_object_parse_affect_value_wrap,
        olc_object_first_affect_ref,
        olc_object_last_affect_ref
    );
}

bool olc_object_in_edit_mode(CHAR_DATA* ch)
{
    return ch
        && ch->desc
        && ch->desc->olc
        && ch->desc->olc->schema
        && ch->desc->olc->schema->name
        && !str_cmp(ch->desc->olc->schema->name, "object");
}

bool olc_object_in_inline_mode(CHAR_DATA* ch)
{
    return ch
        && ch->desc
        && ch->desc->olc
        && ch->desc->olc->mode == OlcEditMode::OBJECT_INLINE;
}

bool olc_object_is_field_name(const std::string& command)
{
    if (command.empty())
        return false;

    return olc_find_field_fuzzy(get_object_schema(), command) != nullptr;
}

void olc_object_edit_help(CHAR_DATA* ch)
{
    send_to_char("OEDIT commands:\n", ch);
    send_to_char("  show [field]\n", ch);
    send_to_char("  help/? [field]\n", ch);
    send_to_char("  commit\n", ch);
    send_to_char("  revert\n", ch);
    send_to_char("  stop save\n", ch);
    send_to_char("  stop abort\n", ch);
    send_to_char("    or just save/abort\n", ch);            
    send_to_char("  <field> <value>\n", ch);    
    return;
}

bool olc_object_edit_interpret(CHAR_DATA* ch, const std::string& command, const std::string& argument)
{
    char buf[MSL];
    SPRINTF(buf, "%s", argument.c_str());
    if (!olc_object_in_inline_mode(ch) || command.empty())
        return false;

    if (olc_is_direction_alias(command.c_str()))
        return false;

    if (olc_command_matches(command, "help") || !str_cmp(command, "?"))
    {
        
        if (argument.empty())
            olc_object_edit_help(ch);
        else
        {
            SPRINTF(buf, "%s %s", "help", argument.c_str() );
            do_oedit(ch, buf);
        }
        return true;
    }

    if (olc_command_matches(command, "stop"))
    {
        std::string arg;
        one_argument(argument, arg);

        if (!str_cmp(arg, "save"))
        {
            olc_stop(ch, true);
            return true;
        }

        if (!str_cmp(arg, "abort"))
        {
            olc_stop(ch, false);
            return true;
        }

        send_to_char("Syntax: stop save|abort\n", ch);
        return true;
    }

    if (!str_cmp(command, "abort") || !str_cmp(command, "cancel"))
    {
        olc_stop(ch, false);
        return true;
    }

    if (!str_cmp(command, "save"))
    {
        olc_stop(ch, true);
        return true;
    }
    
    if (olc_command_matches(command, "revert"))
    {
        olc_object_edit_revert(ch);
        return true;
    }

    if (olc_command_matches(command, "commit"))
    {
        if (olc_commit_current(ch))
            send_to_char("Current changes committed.\n", ch);
        else
            send_to_char("No pending changes to commit.\n", ch);
        return true;
    }

    if (olc_command_matches(command, "show"))
    {
        do_olcshow(ch, buf);
        return true;
    }

    if (olc_command_matches(command, "olcshow"))
    {
        do_olcshow(ch, buf);
        return true;
    }

    if (olc_command_matches(command, "look"))
    {
        return false;
    }

    if (olc_command_matches(command, "olcset"))
    {
        do_olcset(ch, buf);
        return true;
    }

    if (olc_object_is_field_name(command))
    {
        snprintf(buf, sizeof(buf), "%s %s", command.c_str(), argument.empty() ? "" : argument.c_str());
        do_olcset(ch, buf);
        return true;
    }

    return false;
}


// --------------------------------------------
// MOBILE_OLC mobile_olc OlcOps functions
// --------------------------------------------
void* mobile_olc_clone(const void* src)
{
    return olc_mobile_clone(static_cast<const CHAR_DATA*>(src));
}

void mobile_olc_free_clone(void* obj)
{
    olc_mobile_free(static_cast<CHAR_DATA*>(obj));
}

void mobile_olc_apply_changes(void* original, void* working)
{
    olc_mobile_apply_instance_changes(
        static_cast<CHAR_DATA*>(original),
        static_cast<const CHAR_DATA*>(working));
}

void mobile_olc_save(CHAR_DATA* ch, void* original)
{
    CHAR_DATA* mob = static_cast<CHAR_DATA*>(original);

    if (!ch || !mob)
        return;

    if (IS_NPC(mob) && BV_IS_SET(mob->act, ACT_PROTOTYPE))
        send_to_char("Prototype-backed mobile changes staged. Save area after commit if needed.\n", ch);
    else
        send_to_char("Live mobile changes saved to instance only.\n", ch);
}

void mobile_olc_after_commit(CHAR_DATA* ch, void* /*original*/, void* /*working*/)
{
    /*
     * IMPORTANT:
     * Prototype delta sync is NOT done here.
     * It must be done in the main commit path where baseline is available.
     */
    if (ch)
        send_to_char("Mobile committed.\n", ch);
}

void mobile_olc_after_revert(CHAR_DATA* ch, void* /*original*/, void* /*working*/)
{
    if (ch)
        send_to_char("Mobile reverted to pre-edit state.\n", ch);
}


const OlcOps mobile_olc_ops =
{
    mobile_olc_clone,
    mobile_olc_free_clone,
    mobile_olc_apply_changes,
    mobile_olc_save,
    mobile_olc_after_commit,
    mobile_olc_after_revert,
    olc_mobile_edit_interpret,
    OlcInterpretStage::EARLY    
};

// ------------------------
// OLC_SHOW_OBJECT
// ------ OBJECTS ---------
// ------------------------

void olc_show_object_compare_line(
    CHAR_DATA* ch,
    const char* label,
    const std::string& current,
    const std::string& proto,
    bool differs,
    int label_width,
    int term_width,
    const char* current_color = OLC_COL_STRING)
{
    int left_col_width;
    int right_col_width;
    char buf[MSL];
    const char* proto_color = differs ? OLC_COL_PROTO_DIFF : OLC_COL_PROTO_SAME;

    std::string left = current.empty() ? "none" : current;
    std::string right = proto.empty() ? "none" : proto;

    left_col_width = 24;
    right_col_width = term_width - (label_width + 3) - left_col_width - 9;

    if (right_col_width < 12)
        right_col_width = 12;

    if (left_col_width > (int)left.size())
        left_col_width = (int)left.size();

    if (left_col_width < 12)
        left_col_width = 12;

    snprintf(buf, sizeof(buf),
        "%s%-*s%s : %s%-*.*s%s  %sProto:%s %s%-*.*s%s\n",
        OLC_COL_LABEL,
        label_width,
        label,
        OLC_COL_RESET,
        current_color,
        left_col_width,
        left_col_width,
        left.c_str(),
        OLC_COL_RESET,
        OLC_COL_LABEL,
        OLC_COL_RESET,
        proto_color,
        right_col_width,
        right_col_width,
        right.c_str(),
        OLC_COL_RESET
    );

    send_to_char(buf, ch);
}

void olc_show_object_string_compare_block(
    CHAR_DATA* ch,
    const char* label,
    const std::string& current,
    const std::string& proto,
    bool differs,
    int label_width,
    int term_width,
    const char* current_color = OLC_COL_STRING)
{
    auto lines = olc_format_line(
        label,
        current.empty() ? "none" : current,
        current_color,
        label_width,
        term_width
    );

    for (const auto& l : lines)
    {
        send_to_char(l, ch);
        send_to_char("\n", ch);
    }

    /* Only show prototype string value when it differs */
    if (!differs)
        return;

    auto plines = olc_format_line(
        "Prototype",
        proto.empty() ? "none" : proto,
        OLC_COL_PROTO_DIFF,
        label_width,
        term_width
    );

    for (const auto& l : plines)
    {
        send_to_char(l, ch);
        send_to_char("\n", ch);
    }
}

void olc_show_object_compare_block(
    CHAR_DATA* ch,
    const char* label,
    const std::string& current,
    const std::string& proto,
    bool differs,
    int label_width,
    int term_width,
    const char* current_color = OLC_COL_STRING)
{
    const char* proto_color = differs ? OLC_COL_PROTO_DIFF : OLC_COL_PROTO_SAME;

    auto lines = olc_format_line(
        label,
        current.empty() ? "none" : current,
        current_color,
        label_width,
        term_width
    );

    for (const auto& l : lines)
    {
        send_to_char(l, ch);
        send_to_char("\n", ch);
    }

    auto plines = olc_format_line(
        "Prototype",
        proto.empty() ? "none" : proto,
        proto_color,
        label_width,
        term_width
    );

    for (const auto& l : plines)
    {
        send_to_char(l, ch);
        send_to_char("\n", ch);
    }
}

void olc_show_object_extradesc_help(CHAR_DATA* ch)
{
    send_to_char("Usage:\n", ch);
    send_to_char("  extradesc <keyword>     (edit/create)\n", ch);
    send_to_char("  extradesc -<keyword>    (delete)\n", ch);
}

void olc_show_object_value_meanings_block(
    CHAR_DATA* ch,
    const char* label,
    const std::vector<std::string>& pairs,
    bool show_proto,
    const std::vector<std::string>& proto_pairs,
    bool differs,
    int label_width,
    int term_width)
{
    int prefix_len = label_width + 3;
    int available = term_width - prefix_len;
    if (available < 20)
        available = 20;

    auto wrapped = olc_wrap_value_pairs(pairs, available);

    if (wrapped.empty())
        wrapped.push_back("none");

    for (size_t i = 0; i < wrapped.size(); ++i)
    {
        char buf[MSL];
        snprintf(buf, sizeof(buf),
            "%s%-*s%s : %s%s%s",
            OLC_COL_LABEL,
            label_width,
            (i == 0 ? label : ""),
            OLC_COL_RESET,
            OLC_COL_HELP,
            wrapped[i].c_str(),
            OLC_COL_RESET
        );
        send_to_char(buf, ch);
        send_to_char("\n", ch);
    }

    if (!show_proto || !differs)
        return;

    auto pwrapped = olc_wrap_value_pairs(proto_pairs, available);
    if (pwrapped.empty())
        pwrapped.push_back("none");

    for (size_t i = 0; i < pwrapped.size(); ++i)
    {
        char buf[MSL];
        snprintf(buf, sizeof(buf),
            "%s%-*s%s : %s%s%s",
            OLC_COL_LABEL,
            label_width,
            (i == 0 ? "Prototype" : ""),
            OLC_COL_RESET,
            OLC_COL_PROTO_DIFF,
            pwrapped[i].c_str(),
            OLC_COL_RESET
        );
        send_to_char(buf, ch);
        send_to_char("\n", ch);
    }
}

void olc_show_affect_help(CHAR_DATA* ch)
{
    send_to_char("Usage:\n", ch);
    send_to_char("  affect add <location> <modifier>\n", ch);
    send_to_char("  affect #<n> delete\n", ch);
    send_to_char("  affect #<n> location <field>\n", ch);
    send_to_char("  affect #<n> modifier <value>\n", ch);
    send_to_char("  affect #<n> duration (duration)\n", ch);
    send_to_char("  affect #<n> type <sn>\n", ch);
    send_to_char("  affect #<n> bitvector <value>\n", ch);
}

void olc_show_object(CHAR_DATA* ch, OBJ_DATA* obj, bool help_only_mode, int term_width)
{
    if (!ch || !obj)
        return;

    OBJ_INDEX_DATA* proto = obj->pIndexData;
    const int label_width = 11;
    char buf[MSL];

    snprintf(buf, sizeof(buf),
        "%s[Object %d]%s %s%s%s  %sSerial:%s %s%d%s\n",
        OLC_COL_HEADER,
        proto ? proto->vnum : 0,
        OLC_COL_RESET,
        OLC_COL_STRING,
        (obj->short_descr && obj->short_descr[0]) ? obj->short_descr : "(no short)",
        OLC_COL_RESET,
        OLC_COL_LABEL,
        OLC_COL_RESET,
        OLC_COL_INT,
        obj->serial,
        OLC_COL_RESET
    );
    send_to_char(buf, ch);

    if (help_only_mode)
        return;

    olc_show_object_string_compare_block(
        ch, "Keywords",
        obj->name ? obj->name : "",
        proto && proto->name ? proto->name : "",
        !(proto && olc_str_eq(obj->name, proto->name)),
        label_width, term_width, OLC_COL_STRING
    );

    olc_show_object_string_compare_block(
        ch, "Short",
        obj->short_descr ? obj->short_descr : "",
        proto && proto->short_descr ? proto->short_descr : "",
        !(proto && olc_str_eq(obj->short_descr, proto->short_descr)),
        label_width, term_width, OLC_COL_STRING
    );

    olc_show_object_compare_line(
        ch, "Type",
        enum_to_string_flag(obj->item_type, o_types),
        proto ? enum_to_string_flag(proto->item_type, o_types) : "",
        !(proto && obj->item_type == proto->item_type),
        label_width, term_width, OLC_COL_ENUM
    );

    olc_show_object_compare_line(
        ch, "Flags",
        get_flag_field(obj->objflags, obj_flag_table),
        proto ? get_flag_field(proto->objflags, obj_flag_table) : "",
        !(proto && obj->objflags == proto->objflags),
        label_width, term_width, OLC_COL_LIST
    );

    olc_show_object_compare_line(
        ch, "Trigflags",
        get_flag_field(obj->trig_flags, trig_flags),
        proto ? get_flag_field(proto->trig_flags, trig_flags) : "",
        !(proto && obj->trig_flags == proto->trig_flags),
        label_width, term_width, OLC_COL_LIST
    );

    olc_show_object_compare_line(
        ch, "Wear Flags",
        get_flag_field(obj->wear_flags, w_flags),
        proto ? get_flag_field(proto->wear_flags, w_flags) : "",
        !(proto && obj->wear_flags == proto->wear_flags),
        label_width, term_width, OLC_COL_LIST
    );

    olc_show_object_compare_line(
        ch, "Weight",
        get_int_field(obj->weight),
        proto ? get_int_field(proto->weight) : "",
        !(proto && obj->weight == proto->weight),
        label_width, term_width, OLC_COL_INT
    );

    olc_show_object_compare_line(
        ch, "Cost",
        get_int_field(obj->cost),
        proto ? get_int_field(proto->cost) : "",
        !(proto && obj->cost == proto->cost),
        label_width, term_width, OLC_COL_INT
    );

    olc_show_object_compare_line(
        ch, "Level",
        get_int_field(obj->level),
        proto ? get_int_field(proto->level) : "",
        !(proto && obj->level == proto->level),
        label_width, term_width, OLC_COL_INT
    );

    olc_show_object_compare_line(
        ch, "Count",
        get_int_field(obj->count),
        proto ? get_int_field(proto->count) : "",
        !(proto && obj->count == proto->count),
        label_width, term_width, OLC_COL_INT
    );
    /* Instance-only: keep standalone */
    {
        auto lines = olc_format_line("Timer", get_int_field(obj->timer), OLC_COL_INT, label_width, term_width);
        for (const auto& l : lines)
        {
            send_to_char(l, ch);
            send_to_char("\n", ch);
        }
    }

    /* Instance-only: keep standalone */
    {
        auto lines = olc_format_line("Wear Loc", get_int_field(obj->wear_loc), OLC_COL_INT, label_width, term_width);
        for (const auto& l : lines)
        {
            send_to_char(l, ch);
            send_to_char("\n", ch);
        }
    }

    {
        olc_show_object_compare_block(
            ch, "Values",
            olc_object_values_summary(obj),
            proto ? olc_object_proto_values_summary(proto) : "",
            !(proto && olc_object_values_equal_display(obj, proto)),
            label_width, term_width, OLC_COL_INT
        );
    }
    {
        auto current_pairs = olc_object_labeled_value_pairs(obj);
        auto proto_pairs = proto ? olc_object_proto_labeled_value_pairs(proto)
                                : std::vector<std::string>{};

        bool differs = false;

        olc_show_object_value_meanings_block(
            ch,
            "Value Meanings",
            current_pairs,
            proto != nullptr,
            proto_pairs,
            differs,
            label_width,
            term_width
        );
    }

    olc_show_object_string_compare_block(
        ch, "Short",
        obj->short_descr ? obj->short_descr : "",
        proto && proto->short_descr ? proto->short_descr : "",
        !(proto && olc_str_eq(obj->short_descr, proto->short_descr)),
        label_width, term_width, OLC_COL_STRING
    );

    if ((obj->action_desc && obj->action_desc[0] != '\0')
    || (proto && proto->action_desc && proto->action_desc[0] != '\0'))
    {
        olc_show_object_string_compare_block(
            ch, "Action",
            obj->action_desc ? obj->action_desc : "",
            proto && proto->action_desc ? proto->action_desc : "",
            !(proto && olc_str_eq(obj->action_desc, proto->action_desc)),
            label_width, term_width, OLC_COL_STRING
        );
    }

    if (obj->first_extradesc || (proto && proto->first_extradesc))
    {
        olc_show_object_compare_block(
            ch, "Extra Descs",
            olc_object_extradesc_list_summary(obj),
            proto ? olc_object_proto_extradesc_list_summary(proto) : "none",
            !(proto && olc_object_extradescs_equal_proto(obj, proto)),
            label_width, term_width, OLC_COL_LIST
        );
    }

    if (obj->first_affect)
    {
        auto lines = olc_object_format_affect_lines(obj, label_width, term_width);
        for (const auto& l : lines)
        {
            send_to_char(l, ch);
            send_to_char("\n", ch);
        }
    }
    else
    {
        auto lines = olc_format_line("Affects", "none", OLC_COL_LIST, label_width, term_width);
        for (const auto& l : lines)
        {
            send_to_char(l, ch);
            send_to_char("\n", ch);
        }
    }
    if (proto && !olc_affects_equal(obj->first_affect, proto->first_affect))
    {
        auto plines = olc_object_format_proto_affect_lines(proto, label_width, term_width);
        for (const auto& l : plines)
        {
            send_to_char(l, ch);
            send_to_char("\n", ch);
        }
    }    
}

// --------------------------------------------
// OLC_MOBILE olc_mobile specific functions
// --------------------------------------------


extern const flag_name npc_race[];
extern const flag_name act_flags[];
extern const flag_name aff_flags[];
extern const flag_name vip_flag_table[];

AFFECT_DATA*& olc_mobile_first_affect_ref(void* obj)
{
    return static_cast<CHAR_DATA*>(obj)->first_affect;
}

AFFECT_DATA*& olc_mobile_last_affect_ref(void* obj)
{
    return static_cast<CHAR_DATA*>(obj)->last_affect;
}

AFFECT_DATA* olc_mobile_find_affect_by_number_wrap(void* obj, int index)
{
    return olc_mobile_find_affect_by_number(static_cast<CHAR_DATA*>(obj), index);
}

bool olc_mobile_parse_affect_value_wrap(CHAR_DATA* ch, int loc, const std::string& value_text, int& out_value)
{
    return olc_mobile_parse_affect_value(ch, loc, value_text, out_value);
}

std::vector<std::string> olc_mobile_format_single_affect_lines(
    AFFECT_DATA* paf,
    size_t max_name_len,
    int term_width)
{
    std::vector<std::string> out;

    if (!paf)
        return out;

    char locbuf[MSL];
    char modbuf[MSL];

    snprintf(locbuf, sizeof(locbuf), "%d", paf->location);
    snprintf(modbuf, sizeof(modbuf), "%d", paf->modifier);

    olc_append_lines(out, olc_format_line("location", locbuf, OLC_COL_ENUM, (int)max_name_len, term_width));
    olc_append_lines(out, olc_format_line("modifier", modbuf, OLC_COL_INT, (int)max_name_len, term_width));

    return out;
}

bool olc_mobile_set_spec(CHAR_DATA* mob, const std::string& value)
{
    if (!mob)
        return false;

    if (!str_cmp(value, "none"))
    {
        mob->spec_fun = nullptr;
        return true;
    }

    SPEC_FUN* spec = spec_lookup(value);
    if (!spec)
        return false;

    mob->spec_fun = spec;
    return true;
}

bool olc_mobile_set_spec2(CHAR_DATA* mob, const std::string& value)
{
    if (!mob)
        return false;

    if (!str_cmp(value, "none"))
    {
        mob->spec_2 = nullptr;
        return true;
    }

    SPEC_FUN* spec = spec_lookup(value);
    if (!spec)
        return false;

    mob->spec_2 = spec;
    return true;
}

bool olc_mobile_set_legacy_bits_field(CHAR_DATA* mob, int CHAR_DATA::*member,
                                          const std::string& value,
                                          const char* const* table)
{
    if (!mob)
        return false;

    int bits = mob->*member;
    std::istringstream iss(value);
    std::string token;
    bool changed = false;

    while (iss >> token)
    {
        if (token.empty())
            continue;

        char op = 0;
        if (token[0] == '+' || token[0] == '-' || token[0] == '!')
        {
            op = token[0];
            token.erase(0, 1);
        }

        if (token.empty())
            continue;

        int flag = -1;
        for (int i = 0; table[i] != nullptr; ++i)
        {
            if (!str_cmp(token, table[i]))
            {
                flag = i;
                break;
            }
        }

        if (flag < 0 || flag > 31)
            return false;

        switch (op)
        {
            case '-':
                REMOVE_BIT(bits, (1 << flag));
                break;
            case '!':
                TOGGLE_BIT(bits, (1 << flag));
                break;
            case '+':
            default:
                SET_BIT(bits, (1 << flag));
                break;
        }

        changed = true;
    }

    if (!changed)
        return false;

    mob->*member = bits;
    return true;
}

std::string olc_mobile_get_legacy_bits_field(
    CHAR_DATA* mob,
    int CHAR_DATA::*member,
    const char* const* table)
{
    if (!mob)
        return "";

    return flag_string(mob->*member, (char* const*)table);
}

bool olc_mobile_set_pending_proto_die_field(CHAR_DATA* ch,
                                            const std::string& field,
                                            const std::string& value)
{
    if (!ch || !ch->desc || !ch->desc->olc)
        return false;

    auto sess = ch->desc->olc;
    int v = 0;

    if (!olc_format_parse_strict_int(value, v))
        return false;

    if (v < 0 || v > 32767)
        return false;

    if (field == "hitnodice")
    {
        sess->pending_mob_proto.hitnodice = static_cast<sh_int>(v);
        sess->pending_mob_proto.hitnodice_set = true;
        sess->dirty = true;
        return true;
    }

    if (field == "hitsizedice")
    {
        sess->pending_mob_proto.hitsizedice = static_cast<sh_int>(v);
        sess->pending_mob_proto.hitsizedice_set = true;
        sess->dirty = true;
        return true;
    }

    if (field == "damnodice")
    {
        sess->pending_mob_proto.damnodice = static_cast<sh_int>(v);
        sess->pending_mob_proto.damnodice_set = true;
        sess->dirty = true;
        return true;
    }

    if (field == "damsizedice")
    {
        sess->pending_mob_proto.damsizedice = static_cast<sh_int>(v);
        sess->pending_mob_proto.damsizedice_set = true;
        sess->dirty = true;
        return true;
    }

    return false;
}


std::string olc_mobile_get_pending_proto_die_field(CHAR_DATA* ch,
                                                   CHAR_DATA* mob,
                                                   const std::string& field)
{
    if (!ch || !ch->desc || !ch->desc->olc)
        return "0";

    auto sess = ch->desc->olc;
    auto& p = sess->pending_mob_proto;

    MOB_INDEX_DATA* idx = (mob ? mob->pIndexData : nullptr);

    if (field == "hitnodice")
    {
        if (p.hitnodice_set)
            return std::to_string(p.hitnodice);
        return idx ? std::to_string(idx->hitnodice) : "0";
    }

    if (field == "hitsizedice")
    {
        if (p.hitsizedice_set)
            return std::to_string(p.hitsizedice);
        return idx ? std::to_string(idx->hitsizedice) : "0";
    }

    if (field == "damnodice")
    {
        if (p.damnodice_set)
            return std::to_string(p.damnodice);
        return idx ? std::to_string(idx->damnodice) : "0";
    }

    if (field == "damsizedice")
    {
        if (p.damsizedice_set)
            return std::to_string(p.damsizedice);
        return idx ? std::to_string(idx->damsizedice) : "0";
    }

    return "0";
}

bool olc_mobile_apply_pending_prototype_changes(CHAR_DATA* ch, CHAR_DATA* live)
{
    if (!ch || !ch->desc || !ch->desc->olc || !live)
        return false;

    if (!IS_NPC(live) || !live->pIndexData)
        return false;

    /*
     * Only apply if the committed live mob still has prototype on.
     */
    if (!BV_IS_SET(live->act, ACT_PROTOTYPE))
        return false;

    auto& p = ch->desc->olc->pending_mob_proto;
    MOB_INDEX_DATA* idx = live->pIndexData;
    bool changed = false;

    if (p.hitnodice_set && idx->hitnodice != p.hitnodice)
    {
        idx->hitnodice = p.hitnodice;
        changed = true;
    }

    if (p.hitsizedice_set && idx->hitsizedice != p.hitsizedice)
    {
        idx->hitsizedice = p.hitsizedice;
        changed = true;
    }

    if (p.damnodice_set && idx->damnodice != p.damnodice)
    {
        idx->damnodice = p.damnodice;
        changed = true;
    }

    if (p.damsizedice_set && idx->damsizedice != p.damsizedice)
    {
        idx->damsizedice = p.damsizedice;
        changed = true;
    }

    return changed;
}

void olc_mobile_init_pending_proto_from_live(OlcSession* sess, CHAR_DATA* mob)
{
    if (!sess)
        return;

    /* Clear first */
    sess->pending_mob_proto.hitnodice_set = false;
    sess->pending_mob_proto.hitsizedice_set = false;
    sess->pending_mob_proto.damnodice_set = false;
    sess->pending_mob_proto.damsizedice_set = false;

    sess->pending_mob_proto.hitnodice = 0;
    sess->pending_mob_proto.hitsizedice = 0;
    sess->pending_mob_proto.damnodice = 0;
    sess->pending_mob_proto.damsizedice = 0;

    if (!mob || !IS_NPC(mob) || !mob->pIndexData)
        return;

    sess->pending_mob_proto.hitnodice = mob->pIndexData->hitnodice;
    sess->pending_mob_proto.hitsizedice = mob->pIndexData->hitsizedice;
    sess->pending_mob_proto.damnodice = mob->pIndexData->damnodice;
    sess->pending_mob_proto.damsizedice = mob->pIndexData->damsizedice;
}

void olc_mobile_reset_pending_proto(OlcSession* sess)
{
    if (!sess)
        return;

    sess->pending_mob_proto.hitnodice_set = false;
    sess->pending_mob_proto.hitsizedice_set = false;
    sess->pending_mob_proto.damnodice_set = false;
    sess->pending_mob_proto.damsizedice_set = false;

    /*
     * Leave stored values as-is; getters can still fall back to index data
     * when *_set is false.
     */
}

bool olc_mobile_set_long_desc(char*& field, const std::string& value)
{
    std::string out = value;

    if (!out.empty())
    {
        while (!out.empty() &&
              (out.back() == '\n' || out.back() == '\r'))
            out.pop_back();

        out += "\n";
    }

    return set_str_field(field, out);
}

bool olc_mobile_set_level(CHAR_DATA* mob, const std::string& value)
{
    int level = 0;

    if (!mob)
        return false;

    if (!set_int_field(level, value, 0, LEVEL_AVATAR + 5))
        return false;

    mob->top_level = level;

    for (int ability = 0; ability < MAX_ABILITY; ++ability)
        mob->skill_level[ability] = level;

    mob->armor   = static_cast<sh_int>(100 - level * 2.5);
    mob->hitroll = static_cast<sh_int>(level / 5);
    mob->damroll = static_cast<sh_int>(level / 5);
    mob->hitplus = static_cast<sh_int>(level * 10);
    mob->damplus = 2;

    return true;
}

bool olc_mobile_set_race(CHAR_DATA* mob, const std::string& value)
{
    int race = get_npc_race(value);

    if (!mob)
        return false;

    if (race < 0)
    {
        if (!olc_format_parse_strict_int(value, race))
            return false;
    }

    if (race < 0 || race >= MAX_NPC_RACE)
        return false;

    mob->race = static_cast<sh_int>(race);
    return true;
}

bool olc_mobile_set_speaking(CHAR_DATA* mob, const std::string& value)
{
    int lang = get_langflag(value);

    if (!mob)
        return false;

    if (lang == LANG_UNKNOWN)
        return false;

    mob->speaking = lang;
    return true;
}

bool olc_mobile_set_long_field(CHAR_DATA* mob, const std::string& value)
{
    if (!mob)
        return false;
    return olc_mobile_set_long_desc(mob->long_descr, value);
}

CHAR_DATA* olc_mobile_clone(const CHAR_DATA* src)
{
    if (!src)
        return nullptr;

    CHAR_DATA* dst = nullptr;
    CREATE(dst, CHAR_DATA, 1);

    /*
     * -------------------------
     * Editable / meaningful data
     * -------------------------
     */
    dst->game       = src->game;
    dst->pIndexData = src->pIndexData;

    dst->name        = src->name        ? STRALLOC(src->name)        : STRALLOC("");
    dst->short_descr = src->short_descr ? STRALLOC(src->short_descr) : STRALLOC("");
    dst->long_descr  = src->long_descr  ? STRALLOC(src->long_descr)  : STRALLOC("");
    dst->description = src->description ? STRALLOC(src->description) : STRALLOC("");

    dst->sex         = src->sex;
    dst->race        = src->race;
    dst->top_level   = src->top_level;

    for (int i = 0; i < MAX_ABILITY; ++i)
        dst->skill_level[i] = src->skill_level[i];

    dst->gold        = src->gold;

    dst->act         = src->act;
    dst->affected_by = src->affected_by;
    dst->xflags      = src->xflags;
    dst->resistant   = src->resistant;
    dst->immune      = src->immune;
    dst->susceptible = src->susceptible;
    dst->attacks     = src->attacks;
    dst->defenses    = src->defenses;
    dst->speaks      = src->speaks;
    dst->speaking    = src->speaking;
    dst->vip_flags   = src->vip_flags;

    dst->saving_poison_death = src->saving_poison_death;
    dst->saving_wand         = src->saving_wand;
    dst->saving_para_petri   = src->saving_para_petri;
    dst->saving_breath       = src->saving_breath;
    dst->saving_spell_staff  = src->saving_spell_staff;

    dst->alignment   = src->alignment;
    dst->barenumdie  = src->barenumdie;
    dst->baresizedie = src->baresizedie;
    dst->mobthac0    = src->mobthac0;
    dst->hitroll     = src->hitroll;
    dst->damroll     = src->damroll;
    dst->hitplus     = src->hitplus;
    dst->damplus     = src->damplus;
    dst->position    = src->position;
    dst->defposition = src->defposition;
    dst->height      = src->height;
    dst->weight      = src->weight;
    dst->armor       = src->armor;
    dst->numattacks  = src->numattacks;

    dst->perm_str    = src->perm_str;
    dst->perm_int    = src->perm_int;
    dst->perm_wis    = src->perm_wis;
    dst->perm_dex    = src->perm_dex;
    dst->perm_con    = src->perm_con;
    dst->perm_cha    = src->perm_cha;
    dst->perm_lck    = src->perm_lck;
    dst->perm_frc    = src->perm_frc;

    dst->mod_str     = src->mod_str;
    dst->mod_int     = src->mod_int;
    dst->mod_wis     = src->mod_wis;
    dst->mod_dex     = src->mod_dex;
    dst->mod_con     = src->mod_con;
    dst->mod_cha     = src->mod_cha;
    dst->mod_lck     = src->mod_lck;
    dst->mod_frc     = src->mod_frc;

    dst->spec_fun    = src->spec_fun;
    dst->spec_2      = src->spec_2;

    dst->hit         = src->hit;
    dst->max_hit     = src->max_hit;
    dst->mana        = src->mana;
    dst->max_mana    = src->max_mana;
    dst->move        = src->move;
    dst->max_move    = src->max_move;
    dst->carry_weight = src->carry_weight;

    dst->first_affect = olc_clone_affects(src->first_affect);
    dst->last_affect = nullptr;
    for (AFFECT_DATA* af = dst->first_affect; af; af = af->next)
        dst->last_affect = af;

    /*
     * -------------------------
     * Runtime/live-world state
     * Leave detached / inert
     * -------------------------
     */
    dst->next = nullptr;
    dst->prev = nullptr;
    dst->next_in_room = nullptr;
    dst->prev_in_room = nullptr;
    dst->master = nullptr;
    dst->leader = nullptr;
    dst->fighting = nullptr;
    dst->reply = nullptr;
    dst->retell = nullptr;
    dst->switched = nullptr;
    dst->mount = nullptr;
    dst->hunting = nullptr;
    dst->fearing = nullptr;
    dst->hating = nullptr;
    dst->mpact = nullptr;
    dst->mpactnum = 0;
    dst->mpscriptpos = 0;
    dst->desc = nullptr;
    dst->first_carrying = nullptr;
    dst->last_carrying = nullptr;
    dst->in_room = nullptr;
    dst->was_in_room = nullptr;
    dst->was_sentinel = nullptr;
    dst->plr_home = nullptr;
    dst->pcdata = nullptr;
    dst->last_cmd = nullptr;
    dst->prev_cmd = nullptr;
    dst->dest_buf = nullptr;
    dst->dest_buf_2 = nullptr;
    dst->spare_ptr = nullptr;
    dst->editor = nullptr;
    dst->first_timer = nullptr;
    dst->last_timer = nullptr;
    dst->guard_data = nullptr;
    dst->buzzed_home = nullptr;
    dst->buzzed_from_room = nullptr;
    dst->questgiver = nullptr;
    dst->challenged = nullptr;
    dst->betted_on = nullptr;
    dst->owner = nullptr;
    dst->home = nullptr;
    dst->on = nullptr;
    dst->pet = nullptr;

    dst->num_fighting = 0;
    dst->substate = SUB_NONE;
    dst->trust = 0;
    dst->timer = 0;
    dst->wait = 0;

    dst->carry_number = 0;
    dst->wimpy = 0;
    dst->mental_state = 0;
    dst->emotional_state = 0;
    dst->pagelen = 0;
    dst->inter_page = 0;
    dst->inter_type = 0;
    dst->inter_editing = nullptr;
    dst->inter_editing_vnum = 0;
    dst->inter_substate = 0;
    dst->retran = 0;
    dst->regoto = 0;
    dst->mobinvis = 0;
    dst->backup_wait = 0;
    dst->backup_mob = 0;
    dst->was_stunned = 0;
    dst->mob_clan = nullptr;
    dst->main_ability = src->main_ability;
    dst->questmob = 0;
    dst->questobj = 0;
    dst->questpoints = 0;
    dst->nextquest = 0;
    dst->countdown = 0;
    dst->bet_amt = 0;
    dst->cmd_recurse = 0;

    return dst;
}

void olc_mobile_free(CHAR_DATA* mob)
{
    if (!mob)
        return;

    if (mob->first_affect)
        olc_free_affects(mob->first_affect);

    if (mob->name)
        STRFREE(mob->name);
    if (mob->short_descr)
        STRFREE(mob->short_descr);
    if (mob->long_descr)
        STRFREE(mob->long_descr);
    if (mob->description)
        STRFREE(mob->description);

    if (mob->inter_editing)
        STRFREE(mob->inter_editing);
    if (mob->mob_clan)
        STRFREE(mob->mob_clan);
    if (mob->owner)
        STRFREE(mob->owner);

    DISPOSE(mob);
}

void olc_mobile_apply_instance_changes(CHAR_DATA* dst, const CHAR_DATA* src)
{
    if (!dst || !src)
        return;

    olc_replace_string(dst->name, src->name);
    olc_replace_string(dst->short_descr, src->short_descr);
    olc_replace_string(dst->long_descr, src->long_descr);
    olc_replace_string(dst->description, src->description);

    dst->sex         = src->sex;
    dst->race        = src->race;
    dst->top_level   = src->top_level;

    for (int i = 0; i < MAX_ABILITY; ++i)
        dst->skill_level[i] = src->skill_level[i];

    dst->gold        = src->gold;

    dst->act         = src->act;
    dst->affected_by = src->affected_by;
    dst->xflags      = src->xflags;
    dst->resistant   = src->resistant;
    dst->immune      = src->immune;
    dst->susceptible = src->susceptible;
    dst->attacks     = src->attacks;
    dst->defenses    = src->defenses;
    dst->speaks      = src->speaks;
    dst->speaking    = src->speaking;
    dst->vip_flags   = src->vip_flags;

    dst->saving_poison_death = src->saving_poison_death;
    dst->saving_wand         = src->saving_wand;
    dst->saving_para_petri   = src->saving_para_petri;
    dst->saving_breath       = src->saving_breath;
    dst->saving_spell_staff  = src->saving_spell_staff;

    dst->alignment   = src->alignment;
    dst->barenumdie  = src->barenumdie;
    dst->baresizedie = src->baresizedie;
    dst->mobthac0    = src->mobthac0;
    dst->hitroll     = src->hitroll;
    dst->damroll     = src->damroll;
    dst->hitplus     = src->hitplus;
    dst->damplus     = src->damplus;
    dst->position    = src->position;
    dst->defposition = src->defposition;
    dst->height      = src->height;
    dst->weight      = src->weight;
    dst->armor       = src->armor;
    dst->numattacks  = src->numattacks;

    dst->perm_str    = src->perm_str;
    dst->perm_int    = src->perm_int;
    dst->perm_wis    = src->perm_wis;
    dst->perm_dex    = src->perm_dex;
    dst->perm_con    = src->perm_con;
    dst->perm_cha    = src->perm_cha;
    dst->perm_lck    = src->perm_lck;
    dst->perm_frc    = src->perm_frc;

    dst->spec_fun    = src->spec_fun;
    dst->spec_2      = src->spec_2;

    dst->max_hit     = src->max_hit;
    dst->max_mana    = src->max_mana;
    dst->max_move    = src->max_move;
    dst->carry_weight = src->carry_weight;

    if (dst->first_affect)
        olc_free_affects(dst->first_affect);

    dst->first_affect = olc_clone_affects(src->first_affect);
    dst->last_affect = nullptr;
    for (AFFECT_DATA* af = dst->first_affect; af; af = af->next)
        dst->last_affect = af;

}

bool olc_mobile_apply_prototype_changes(MOB_INDEX_DATA* idx,
                                    const CHAR_DATA* baseline,
                                    const CHAR_DATA* edited)
{
    bool changed = false;

    if (!idx || !baseline || !edited)
        return false;

    if (!same_str(baseline->name, edited->name))
    {
        olc_replace_string_idx(idx->player_name, edited->name);
        changed = true;
    }

    if (!same_str(baseline->short_descr, edited->short_descr))
    {
        olc_replace_string_idx(idx->short_descr, edited->short_descr);
        changed = true;
    }

    if (!same_str(baseline->long_descr, edited->long_descr))
    {
        olc_replace_string_idx(idx->long_descr, edited->long_descr);
        changed = true;
    }

    if (!same_str(baseline->description, edited->description))
    {
        olc_replace_string_idx(idx->description, edited->description);
        changed = true;
    }

    if (baseline->sex != edited->sex)
    {
        idx->sex = edited->sex;
        changed = true;
    }

    if (baseline->top_level != edited->top_level)
    {
        idx->level = edited->top_level;
        changed = true;
    }

    if (baseline->act != edited->act)
    {
        idx->act = edited->act;
        changed = true;
    }

    if (baseline->affected_by != edited->affected_by)
    {
        idx->affected_by = edited->affected_by;
        changed = true;
    }

    if (baseline->alignment != edited->alignment)
    {
        idx->alignment = edited->alignment;
        changed = true;
    }

    if (baseline->armor != edited->armor)
    {
        idx->ac = edited->armor;
        changed = true;
    }

    /*
     * Intentionally NOT syncing:
     *   hitnodice / hitsizedice / damnodice / damsizedice
     * until live CHAR_DATA has clear dedicated fields for them.
     */

    if (baseline->hitplus != edited->hitplus)
    {
        idx->hitplus = edited->hitplus;
        changed = true;
    }

    if (baseline->damplus != edited->damplus)
    {
        idx->damplus = edited->damplus;
        changed = true;
    }

    if (baseline->numattacks != edited->numattacks)
    {
        idx->numattacks = edited->numattacks;
        changed = true;
    }

    if (baseline->gold != edited->gold)
    {
        idx->gold = edited->gold;
        changed = true;
    }

    if (baseline->xflags != edited->xflags)
    {
        idx->xflags = edited->xflags;
        changed = true;
    }

    if (baseline->resistant != edited->resistant)
    {
        idx->resistant = edited->resistant;
        changed = true;
    }

    if (baseline->immune != edited->immune)
    {
        idx->immune = edited->immune;
        changed = true;
    }

    if (baseline->susceptible != edited->susceptible)
    {
        idx->susceptible = edited->susceptible;
        changed = true;
    }

    if (baseline->attacks != edited->attacks)
    {
        idx->attacks = edited->attacks;
        changed = true;
    }

    if (baseline->defenses != edited->defenses)
    {
        idx->defenses = edited->defenses;
        changed = true;
    }

    if (baseline->speaks != edited->speaks)
    {
        idx->speaks = edited->speaks;
        changed = true;
    }

    if (baseline->speaking != edited->speaking)
    {
        idx->speaking = edited->speaking;
        changed = true;
    }

    if (baseline->position != edited->position)
    {
        idx->position = edited->position;
        changed = true;
    }

    if (baseline->defposition != edited->defposition)
    {
        idx->defposition = edited->defposition;
        changed = true;
    }

    if (baseline->height != edited->height)
    {
        idx->height = edited->height;
        changed = true;
    }

    if (baseline->weight != edited->weight)
    {
        idx->weight = edited->weight;
        changed = true;
    }

    if (baseline->race != edited->race)
    {
        idx->race = edited->race;
        changed = true;
    }

    if (baseline->hitroll != edited->hitroll)
    {
        idx->hitroll = edited->hitroll;
        changed = true;
    }

    if (baseline->damroll != edited->damroll)
    {
        idx->damroll = edited->damroll;
        changed = true;
    }

    if (baseline->perm_str != edited->perm_str)
    {
        idx->perm_str = edited->perm_str;
        changed = true;
    }

    if (baseline->perm_int != edited->perm_int)
    {
        idx->perm_int = edited->perm_int;
        changed = true;
    }

    if (baseline->perm_wis != edited->perm_wis)
    {
        idx->perm_wis = edited->perm_wis;
        changed = true;
    }

    if (baseline->perm_dex != edited->perm_dex)
    {
        idx->perm_dex = edited->perm_dex;
        changed = true;
    }

    if (baseline->perm_con != edited->perm_con)
    {
        idx->perm_con = edited->perm_con;
        changed = true;
    }

    if (baseline->perm_cha != edited->perm_cha)
    {
        idx->perm_cha = edited->perm_cha;
        changed = true;
    }

    if (baseline->perm_lck != edited->perm_lck)
    {
        idx->perm_lck = edited->perm_lck;
        changed = true;
    }

    if (baseline->perm_frc != edited->perm_frc)
    {
        idx->perm_frc = edited->perm_frc;
        changed = true;
    }

    if (baseline->saving_poison_death != edited->saving_poison_death)
    {
        idx->saving_poison_death = edited->saving_poison_death;
        changed = true;
    }

    if (baseline->saving_wand != edited->saving_wand)
    {
        idx->saving_wand = edited->saving_wand;
        changed = true;
    }

    if (baseline->saving_para_petri != edited->saving_para_petri)
    {
        idx->saving_para_petri = edited->saving_para_petri;
        changed = true;
    }

    if (baseline->saving_breath != edited->saving_breath)
    {
        idx->saving_breath = edited->saving_breath;
        changed = true;
    }

    if (baseline->saving_spell_staff != edited->saving_spell_staff)
    {
        idx->saving_spell_staff = edited->saving_spell_staff;
        changed = true;
    }

    if (baseline->vip_flags != edited->vip_flags)
    {
        idx->vip_flags = edited->vip_flags;
        changed = true;
    }

    if (baseline->spec_fun != edited->spec_fun)
    {
        idx->spec_fun = edited->spec_fun;
        changed = true;
    }

    if (baseline->spec_2 != edited->spec_2)
    {
        idx->spec_2 = edited->spec_2;
        changed = true;
    }

    return changed;
}

bool olc_mobile_maybe_sync_prototype(CHAR_DATA* live,
                                 const CHAR_DATA* baseline,
                                 const CHAR_DATA* edited)
{
    if (!live || !baseline || !edited)
        return false;

    if (!IS_NPC(live) || !live->pIndexData)
        return false;

    /*
     * Decide prototype propagation from the POST-COMMIT live mob flags.
     */
    if (!BV_IS_SET(live->act, ACT_PROTOTYPE))
        return false;

    return olc_mobile_apply_prototype_changes(live->pIndexData, baseline, edited);
}

bool olc_mobile_edit_revert(CHAR_DATA* ch)
{
    if (!ch || !ch->desc || !ch->desc->olc)
        return false;

    auto sess = ch->desc->olc;

    if (!sess->schema || !sess->schema->name ||
        str_cmp(sess->schema->name, "mobile"))
        return false;

    if (!sess->original_clone || !sess->ops ||
        !sess->ops->clone || !sess->ops->free_clone)
    {
        send_to_char("No revert snapshot is available.\n", ch);
        return false;
    }

    if (sess->working_copy)
        sess->ops->free_clone(sess->working_copy);

    sess->working_copy = sess->ops->clone(sess->original_clone);
    if (!sess->working_copy)
    {
        send_to_char("Failed to restore revert snapshot.\n", ch);
        return false;
    }

    sess->last_cmd_arg.clear();
    sess->dirty = false;

    olc_mobile_reset_pending_proto(sess);
    olc_mobile_init_pending_proto_from_live(
        sess,
        static_cast<CHAR_DATA*>(sess->original));

    if (sess->ops->after_revert)
        sess->ops->after_revert(ch, sess->original, sess->working_copy);

    send_to_char("Mobile reverted to pre-edit state.\n", ch);
    return true;
}

bool olc_mobile_in_edit_mode(CHAR_DATA* ch)
{
    return ch && ch->desc && ch->desc->olc &&
           ch->desc->olc->schema &&
           ch->desc->olc->schema->name &&
           !str_cmp(ch->desc->olc->schema->name, "mobile");
}

void olc_mobile_edit_help(CHAR_DATA* ch)
{
    send_to_char("MEDIT commands:\n", ch);
    send_to_char("  show [field]\n", ch);
    send_to_char("  help/? [field]\n", ch);
    send_to_char("  commit\n", ch);
    send_to_char("  revert\n", ch);
    send_to_char("  stop save\n", ch);
    send_to_char("  stop abort\n", ch);
    send_to_char("    or save/abort", ch);
    send_to_char("  <field> <value>\n", ch);
}

bool olc_mobile_edit_interpret(CHAR_DATA* ch, const std::string& command, const std::string& argument)
{
    char buf[MSL];
    SPRINTF(buf, "%s", argument.c_str());
    if (!olc_mobile_in_edit_mode(ch) || command.empty())
        return false;

    if (olc_command_matches(command, "help") || !str_cmp(command, "?"))
    {
        SPRINTF(buf, "%s %s", "help", argument.c_str());
        do_medit(ch, buf);
        return true;
    }

    if (olc_command_matches(command, "stop"))
    {
        std::string arg;
        one_argument(argument, arg);

        if (!str_cmp(arg, "save"))
        {
            olc_stop(ch, true);
            return true;
        }

        if (!str_cmp(arg, "abort"))
        {
            olc_stop(ch, false);
            return true;
        }

        send_to_char("Syntax: stop save|abort\n", ch);
        return true;
    }

    if (!str_cmp(command, "abort") || !str_cmp(command, "cancel"))
    {
        olc_stop(ch, false);
        return true;
    }

    if (!str_cmp(command, "save"))
    {
        olc_stop(ch, true);
        return true;
    }    

    if (olc_command_matches(command, "revert"))
    {
        olc_mobile_edit_revert(ch);
        return true;
    }

    if (olc_command_matches(command, "commit"))
    {
        if (olc_commit_current(ch))
            send_to_char("Current changes committed.\n", ch);
        else
            send_to_char("No pending changes to commit.\n", ch);
        return true;
    }

    if (olc_command_matches(command, "show"))
    {
        do_olcshow(ch, buf);
        return true;
    }

    if (olc_command_matches(command, "olcshow"))
    {
        do_olcshow(ch, buf);
        return true;
    }

    if (olc_command_matches(command, "look"))
        return false;

    if (olc_command_matches(command, "olcset"))
    {
        do_olcset(ch, buf);
        return true;
    }

    if (olc_session_command_is_field_name(ch->desc->olc, command))
    {
        snprintf(buf, sizeof(buf), "%s %s", command.c_str(), !argument.empty() ? argument.c_str() : "");
        do_olcset(ch, buf);
        return true;
    }

    return false;
}

bool olc_mobile_parse_affect_value(
    CHAR_DATA* ch,
    int loc,
    const std::string& value_text,
    int& out_value)
{
    if (loc == APPLY_AFFECT)
    {
        out_value = get_aflag(value_text);
        if (out_value < 0)
        {
            ch_printf(ch, "Unknown affect flag: %s\n", value_text.c_str());
            return false;
        }
        return true;
    }

    if (loc > APPLY_AFFECT && loc < APPLY_WEAPONSPELL)
    {
        out_value = get_risflag(value_text);
        if (out_value < 0)
        {
            ch_printf(ch, "Unknown resistance flag: %s\n", value_text.c_str());
            return false;
        }
        return true;
    }

    return olc_format_parse_strict_int(value_text, out_value);
}

AFFECT_DATA* olc_mobile_find_affect_by_number(CHAR_DATA* mob, int index)
{
    if (!mob || index <= 0)
        return nullptr;

    int count = 0;
    for (AFFECT_DATA* af = mob->first_affect; af; af = af->next)
    {
        ++count;
        if (count == index)
            return af;
    }

    return nullptr;
}

std::string olc_mobile_affect_list_summary(CHAR_DATA* mob)
{
    if (!mob || !mob->first_affect)
        return "none";

    int count = 0;
    for (AFFECT_DATA* af = mob->first_affect; af; af = af->next)
        ++count;

    return std::to_string(count) + " affect(s)";
}

std::vector<std::string> olc_mobile_format_affect_lines(
    CHAR_DATA* mob,
    size_t label_width,
    int term_width)
{
    std::vector<std::string> out;

    if (!mob || !mob->first_affect)
        return out;

    bool first_line = true;
    int count = 0;

    for (AFFECT_DATA* af = mob->first_affect; af; af = af->next)
    {
        ++count;

        std::string num = "#" + std::to_string(count) + ":";
        std::string loc = olc_object_affect_location_name(af->location);

        std::string tail =
            "Modifier: " + std::to_string(af->modifier) +
            "  Duration: " + std::to_string(af->duration) +
            "  Type: " + std::to_string(af->type) +
            "  Bitvector: " + (af->bitvector >= 0 ? get_flag_name(aff_flags,af->bitvector, AFF_MAX) : "none");

        char prefix[MSL];
        snprintf(prefix, sizeof(prefix),
            "%s%-*s%s : %s%-4s%s %s%-16s%s  ",
            OLC_COL_LABEL,
            (int)label_width,
            first_line ? "affect" : "",
            OLC_COL_RESET,
            OLC_COL_INT,
            num.c_str(),
            OLC_COL_RESET,
            OLC_COL_ENUM,
            loc.c_str(),
            OLC_COL_RESET
        );

        int prefix_len = visible_length(prefix);
        int available = term_width - prefix_len;
        if (available < 20)
            available = 20;

        auto wrapped = olc_format_wrap_text(tail, available, prefix_len);
        if (wrapped.empty())
            wrapped.push_back(tail);

        out.push_back(std::string(prefix) + OLC_COL_STRING + wrapped[0] + OLC_COL_RESET);

        for (size_t i = 1; i < wrapped.size(); ++i)
        {
            char cont[MSL];
            snprintf(cont, sizeof(cont),
                "%*s   %s%s%s",
                prefix_len,
                "",
                OLC_COL_STRING,
                wrapped[i].c_str(),
                OLC_COL_RESET
            );
            out.push_back(cont);
        }

        first_line = false;
    }

    return out;
}

bool olc_mobile_edit_affect_field(CHAR_DATA* ch, CHAR_DATA* mob)
{
    return olc_edit_affect_field_generic(
        ch,
        mob,
        olc_mobile_find_affect_by_number_wrap,
        olc_mobile_parse_affect_value_wrap,
        olc_mobile_first_affect_ref,
        olc_mobile_last_affect_ref
    );
}

// ------------------------
// OLC_SHOW_MOBILE
// -------MOBILES----------
// ------------------------

void olc_show_mob(CHAR_DATA* ch, CHAR_DATA* mob, bool show_help, int term_width)
{
    if (!ch || !mob)
        return;

    char buf[MSL];

    const int vnum = (mob->pIndexData ? mob->pIndexData->vnum : 0);
    const char* short_descr =
        (mob->short_descr && mob->short_descr[0]) ? mob->short_descr : "(no short)";
    const char* keywords =
        (mob->name && mob->name[0]) ? mob->name : "(no keywords)";
    const char* long_descr =
        (mob->long_descr && mob->long_descr[0]) ? mob->long_descr : "(no long)";
    const char* description =
        (mob->description && mob->description[0]) ? mob->description : "(no long)";        
    const char* race_name =
        (mob->race >= 0 && mob->race < MAX_NPC_RACE)
            ? get_race(mob) : "unknown";

    snprintf(buf, sizeof(buf),
        "%sMobile [%d]%s %s%s%s\n",
        OLC_COL_HEADER,
        vnum,
        OLC_COL_RESET,
        OLC_COL_STRING,
        keywords,
        OLC_COL_RESET);
    send_to_char(buf, ch);

    snprintf(buf, sizeof(buf),
        "%sDescription%s: %s%s%s\n",
        OLC_COL_LABEL,
        OLC_COL_RESET,
        OLC_COL_STRING,
        description,
        OLC_COL_RESET);
    send_to_char(buf, ch);

    snprintf(buf, sizeof(buf),
        "%sShort Desc%s : %s%s%s\n",
        OLC_COL_LABEL,
        OLC_COL_RESET,
        OLC_COL_STRING,
        short_descr,
        OLC_COL_RESET);
    send_to_char(buf, ch);

    snprintf(buf, sizeof(buf),
        "%sLong%s       : %s%s%s\n",
        OLC_COL_LABEL,
        OLC_COL_RESET,
        OLC_COL_STRING,
        long_descr,
        OLC_COL_RESET);
    send_to_char(buf, ch);

    if (long_descr[0] == '\0' || long_descr[strlen(long_descr) - 1] != '\n')
        send_to_char("\n", ch);

    snprintf(buf, sizeof(buf),
        "%s     Stats%s%s : Str: %-2d Dex: %-2d Con: %-2d Int: %-2d Wis: %-2d Cha: %-2d Frc: %-2d Lck: %-2d\n",
        OLC_COL_LABEL,
        OLC_COL_RESET,
        OLC_COL_ENUM,
        mob->perm_str,
        mob->perm_dex,
        mob->perm_con,
        mob->perm_int,
        mob->perm_wis,
        mob->perm_cha,
        mob->perm_frc,
        mob->perm_lck
        );
    send_to_char(buf, ch);

    snprintf(buf, sizeof(buf),
        "%s     %s%s      : HP :%6d/%-6d  FO :%6d/%-6d  MV :%6d/%-6d  \n",
        OLC_COL_LABEL,
        OLC_COL_RESET,
        OLC_COL_ENUM,
        mob->hit,
        mob->max_hit,
        mob->mana,
        mob->max_mana,
        mob->move,
        mob->max_move
        );
    send_to_char(buf, ch);

    snprintf(buf, sizeof(buf),
        "       %sSex%s : %s%d%s   %sLevel%s : %s%d%s   %sRace%s : %s%s%s   \n",

        OLC_COL_LABEL,
        OLC_COL_RESET,
        OLC_COL_INT,
        mob->sex,
        OLC_COL_RESET,
        OLC_COL_LABEL,
        OLC_COL_RESET,
        OLC_COL_INT,
        mob->top_level,
        OLC_COL_RESET,
        OLC_COL_LABEL,
        OLC_COL_RESET,
        OLC_COL_ENUM,
        race_name,
        OLC_COL_RESET);
    send_to_char(buf, ch);

    snprintf(buf, sizeof(buf),
        "%s     Align%s : %s%d%s   %sArmor%s : %s%d%s   %sGold%s  : %s%d%s\n",
        OLC_COL_LABEL,
        OLC_COL_RESET,
        OLC_COL_INT,
        mob->alignment,
        OLC_COL_RESET,
        OLC_COL_LABEL,
        OLC_COL_RESET,
        OLC_COL_INT,
        mob->armor,
        OLC_COL_RESET,
        OLC_COL_LABEL,
        OLC_COL_RESET,
        OLC_COL_INT,
        mob->gold,
        OLC_COL_RESET);
    send_to_char(buf, ch);

    snprintf(buf, sizeof(buf),
        "%s   Hitroll%s : %s%d%s   %sDamroll%s : %s%d%s   %sAttacks%s : %s%d%s\n",
        OLC_COL_LABEL,
        OLC_COL_RESET,
        OLC_COL_INT,
        mob->hitroll,
        OLC_COL_RESET,
        OLC_COL_LABEL,
        OLC_COL_RESET,
        OLC_COL_INT,
        mob->damroll,
        OLC_COL_RESET,
        OLC_COL_LABEL,
        OLC_COL_RESET,
        OLC_COL_INT,
        mob->numattacks,
        OLC_COL_RESET);
    send_to_char(buf, ch);

    snprintf(buf, sizeof(buf),
        "%sSav Throws%s%s : Poi:%2d  Wan:%2d  Par:%2d  Bre:%2d  Spe:%2d\n",
        OLC_COL_LABEL,
        OLC_COL_RESET,
        OLC_COL_ENUM,
        mob->saving_poison_death,
        mob->saving_wand,
        mob->saving_para_petri,
        mob->saving_breath,
        mob->saving_spell_staff
        );
    send_to_char(buf, ch);

    snprintf(buf, sizeof(buf),
        "%s       Pos%s : %s%d%s   %sDefPos%s : %s%d%s   %sSpeak%s : %s%s%s\n",
        OLC_COL_LABEL,
        OLC_COL_RESET,
        OLC_COL_INT,
        mob->position,
        OLC_COL_RESET,
        OLC_COL_LABEL,
        OLC_COL_RESET,
        OLC_COL_INT,
        mob->defposition,
        OLC_COL_RESET,
        OLC_COL_LABEL,
        OLC_COL_RESET,
        OLC_COL_INT,
        lang_names[mob->speaking].name,
        OLC_COL_RESET);
    send_to_char(buf, ch);

    send_to_char("Languages  : ", ch);
    for (int x = 0; x < LANG_MAX; ++x)
    {
        if (!lang_names[x].name)
            continue;
        bool knows = knows_language(mob, x, mob);
        bool speaking = (mob->speaking == x);

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

    snprintf(buf, sizeof(buf),
        "%s      Size%s : %sHt %d%s  %sWt %d%s  %sCarry Limit %d%s\n",
        OLC_COL_LABEL,
        OLC_COL_RESET,
        OLC_COL_INT,
        mob->height,
        OLC_COL_RESET,
        OLC_COL_INT,
        mob->weight,
        OLC_COL_RESET,
        OLC_COL_INT,
        mob->carry_weight,
        OLC_COL_RESET);
    send_to_char(buf, ch);

    /*
     * Prototype-only queued dice values:
     * show pending session values if present, otherwise current index values.
     */
    if (ch->desc && ch->desc->olc)
    {
        std::string hitnodice   = olc_mobile_get_pending_proto_die_field(ch, mob, "hitnodice");
        std::string hitsizedice = olc_mobile_get_pending_proto_die_field(ch, mob, "hitsizedice");
        std::string damnodice   = olc_mobile_get_pending_proto_die_field(ch, mob, "damnodice");
        std::string damsizedice = olc_mobile_get_pending_proto_die_field(ch, mob, "damsizedice");

        snprintf(buf, sizeof(buf),
            "%s   HitDice%s : %s%s d %s%s%s   %sHitPlus%s : %d%s\n",
            OLC_COL_LABEL,
            OLC_COL_RESET,
            OLC_COL_INT,
            hitnodice.c_str(),
            hitsizedice.c_str(),
            OLC_COL_RESET,
            OLC_COL_LABEL,
            OLC_COL_RESET,
            OLC_COL_INT,
            mob->hitplus,
            OLC_COL_RESET);
        send_to_char(buf, ch);

        snprintf(buf, sizeof(buf),
            "%s   DamDice%s : %s%s d %s%s%s   %sDamPlus%s : %d%s\n",
            OLC_COL_LABEL,
            OLC_COL_RESET,
            OLC_COL_INT,
            damnodice.c_str(),
            damsizedice.c_str(),
            OLC_COL_RESET,
            OLC_COL_LABEL,
            OLC_COL_RESET,
            OLC_COL_INT,
            mob->damplus,
            OLC_COL_RESET);
        send_to_char(buf, ch);
    }

    /*
     * Compact flags at the bottom.
     */
    std::string act_flags_str   = get_flag_field(mob->act, act_flags);
    std::string aff_flags_str   = get_flag_field(mob->affected_by, aff_flags);
    std::string vip_flags_str   = get_flag_field(mob->vip_flags, planet_flags);
    std::string part_flags_str  = flag_string(mob->xflags, part_flags);
    std::string res_flags_str   = get_flag_field(mob->resistant, ris_flags);
    std::string imm_flags_str   = get_flag_field(mob->immune, ris_flags);
    std::string sus_flags_str   = get_flag_field(mob->susceptible, ris_flags);
    std::string att_flags_str   = flag_string(mob->attacks, attack_flags);
    std::string def_flags_str   = flag_string(mob->defenses, defense_flags);

    auto act_lines = olc_format_line(
        "flags",
        act_flags_str.empty() ? "none" : act_flags_str,
        OLC_COL_LIST,
        10,
        term_width
    );
    for (const auto& line : act_lines)
        send_to_char((line + "\n"), ch);

    auto aff_lines = olc_format_line(
        "affected",
        aff_flags_str.empty() ? "none" : aff_flags_str,
        OLC_COL_LIST,
        10,
        term_width
    );
    for (const auto& line : aff_lines)
        send_to_char((line + "\n"), ch);

    auto vip_lines = olc_format_line(
        "vip",
        vip_flags_str.empty() ? "none" : vip_flags_str,
        OLC_COL_LIST,
        10,
        term_width
    );
    for (const auto& line : vip_lines)
        send_to_char((line + "\n"), ch);

    auto part_lines = olc_format_line(
        "parts",
        part_flags_str.empty() ? "none" : part_flags_str,
        OLC_COL_LIST,
        10,
        term_width
    );
    for (const auto& line : part_lines)
        send_to_char((line + "\n"), ch);        

    auto res_lines = olc_format_line(
        "resistant",
        res_flags_str.empty() ? "none" : res_flags_str,
        OLC_COL_LIST,
        10,
        term_width
    );
    for (const auto& line : res_lines)
        send_to_char((line + "\n"), ch);        

    auto imm_lines = olc_format_line(
        "immune",
        imm_flags_str.empty() ? "none" : imm_flags_str,
        OLC_COL_LIST,
        10,
        term_width
    );
    for (const auto& line : imm_lines)
        send_to_char((line + "\n"), ch);    

    auto sus_lines = olc_format_line(
        "suscept",
        sus_flags_str.empty() ? "none" : sus_flags_str,
        OLC_COL_LIST,
        10,
        term_width
    );
    for (const auto& line : sus_lines)
        send_to_char((line + "\n"), ch);   

    auto att_lines = olc_format_line(
        "attacks",
        att_flags_str.empty() ? "none" : att_flags_str,
        OLC_COL_LIST,
        10,
        term_width
    );
    for (const auto& line : att_lines)
        send_to_char((line + "\n"), ch);           

    auto def_lines = olc_format_line(
        "defenses",
        def_flags_str.empty() ? "none" : def_flags_str,
        OLC_COL_LIST,
        10,
        term_width
    );
    for (const auto& line : def_lines)
        send_to_char((line + "\n"), ch);          

    if (mob->first_affect)
    {
        auto lines = olc_mobile_format_affect_lines(mob, 10, term_width);
        for (const auto& l : lines)
        {
            send_to_char(l, ch);
            send_to_char("\n", ch);
        }
    }
    else
    {
        auto lines = olc_format_line("Affects", "none", OLC_COL_LIST, 10, term_width);
        for (const auto& l : lines)
        {
            send_to_char(l, ch);
            send_to_char("\n", ch);
        }
    }

}

// --------------------------------------------
// SHOP_OLC shop_olc OlcOps functions
// --------------------------------------------
void* shop_olc_clone(const void* src)
{
    return olc_shop_clone(static_cast<const SHOP_DATA*>(src));
}

void shop_olc_free_clone(void* obj)
{
    olc_shop_free(static_cast<SHOP_DATA*>(obj));
}

void shop_olc_apply_changes(void* original, void* working)
{
    olc_shop_apply_instance_changes(
        static_cast<SHOP_DATA*>(original),
        static_cast<const SHOP_DATA*>(working));
}

void shop_olc_save(CHAR_DATA* ch, void* original)
{
    SHOP_DATA* shop = static_cast<SHOP_DATA*>(original);

    if (!ch || !shop)
        return;

    send_to_char("Live shop changes saved.  You need to foldarea to save changes permanently.\n", ch);
}

void shop_olc_after_commit(CHAR_DATA* ch, void* /*original*/, void* /*working*/)
{
    /*
     * IMPORTANT:
     * Prototype delta sync is NOT done here.
     * It must be done in the main commit path where baseline is available.
     */
    if (ch)
        send_to_char("Shop committed.  You need to foldarea to save changes permanently.\n", ch);
}

void shop_olc_after_revert(CHAR_DATA* ch, void* /*original*/, void* /*working*/)
{
    if (ch)
        send_to_char("Shop reverted to pre-edit state.\n", ch);
}

bool olc_shop_edit_interpret(CHAR_DATA* ch, const std::string& command, const std::string& argument)
{
    char buf[MSL];
    SPRINTF(buf, "%s", argument.c_str());
    if (!olc_shop_in_edit_mode(ch) || command.empty())
        return false;

    if (olc_command_matches(command, "help") || !str_cmp(command, "?"))
    {
        SPRINTF(buf, "%s %s", "help", argument.c_str());
        do_shopset(ch, buf);
        return true;
    }

    if (olc_command_matches(command, "stop"))
    {
        std::string arg;
        one_argument(argument, arg);

        if (!str_cmp(arg, "save"))
        {
            olc_stop(ch, true);
            return true;
        }

        if (!str_cmp(arg, "abort"))
        {
            olc_stop(ch, false);
            return true;
        }

        send_to_char("Syntax: stop save|abort\n", ch);
        return true;
    }

    if (!str_cmp(command, "abort") || !str_cmp(command, "cancel"))
    {
        olc_stop(ch, false);
        return true;
    }

    if (!str_cmp(command, "save"))
    {
        olc_stop(ch, true);
        return true;
    }    

    if (olc_command_matches(command, "revert"))
    {
        olc_shop_edit_revert(ch);
        return true;
    }

    if (olc_command_matches(command, "commit"))
    {
        if (olc_commit_current(ch))
            send_to_char("Current changes committed.\n", ch);
        else
            send_to_char("No pending changes to commit.\n", ch);
        return true;
    }

    if (olc_command_matches(command, "show"))
    {
        do_olcshow(ch, buf);
        return true;
    }

    if (olc_command_matches(command, "olcshow"))
    {
        do_olcshow(ch, buf);
        return true;
    }

    if (olc_command_matches(command, "look"))
        return false;

    if (olc_command_matches(command, "olcset"))
    {
        do_olcset(ch, buf);
        return true;
    }

    if (olc_session_command_is_field_name(ch->desc->olc, command))
    {
        snprintf(buf, sizeof(buf), "%s %s", command.c_str(), argument.empty() ? "" : argument.c_str());
        do_olcset(ch, buf);
        return true;
    }

    return false;
}

const OlcOps shop_olc_ops =
{
    shop_olc_clone,
    shop_olc_free_clone,
    shop_olc_apply_changes,
    shop_olc_save,
    shop_olc_after_commit,
    shop_olc_after_revert,
    olc_shop_edit_interpret,
    OlcInterpretStage::EARLY    
};

// --------------------------------------------
// SHOP_OLC shop_olc other  functions
// --------------------------------------------

bool olc_shop_in_edit_mode(CHAR_DATA* ch)
{
    return ch && ch->desc && ch->desc->olc &&
           ch->desc->olc->schema &&
           ch->desc->olc->schema->name &&
           !str_cmp(ch->desc->olc->schema->name, "shop");
}

void olc_shop_edit_help(CHAR_DATA* ch)
{
    send_to_char("SHOPSET commands:\n", ch);
    send_to_char("  show [field]\n", ch);
    send_to_char("  help/? [field]\n", ch);
    send_to_char("  commit\n", ch);
    send_to_char("  revert\n", ch);
    send_to_char("  stop save\n", ch);
    send_to_char("  stop abort\n", ch);
    send_to_char("    or save/abort", ch);
    send_to_char("  <field> <value>\n", ch);
}

bool olc_shop_edit_revert(CHAR_DATA *ch)
{
    if (!ch || !ch->desc || !ch->desc->olc)
        return false;

    auto sess = ch->desc->olc;

    /* Must be editing a shop */
    if (!sess->schema || !sess->schema->name ||
        str_cmp(sess->schema->name, "shop"))
        return false;

    if (!sess->original_clone || !sess->ops ||
        !sess->ops->clone || !sess->ops->free_clone)
    {
        send_to_char("No revert snapshot is available.\n", ch);
        return false;
    }

    /* Free current working copy */
    if (sess->working_copy)
        sess->ops->free_clone(sess->working_copy);

    /* Restore from original clone */
    sess->working_copy = sess->ops->clone(sess->original_clone);
    if (!sess->working_copy)
    {
        send_to_char("Failed to restore revert snapshot.\n", ch);
        return false;
    }

    sess->last_cmd_arg.clear();
    sess->dirty = false;

    /* IMPORTANT: clear pending exit side effects */
    sess->pending_exit_side_effects.clear();

    if (sess->ops->after_revert)
        sess->ops->after_revert(ch, sess->original, sess->working_copy);

    send_to_char("Shop reverted to pre-edit state.\n", ch);
    return true;
    
}

SHOP_DATA* olc_shop_clone(const SHOP_DATA* src)
{
    if (!src)
        return nullptr;

    SHOP_DATA* dst = nullptr;
    CREATE(dst, SHOP_DATA, 1);

    /*
     * -------------------------
     * Editable / meaningful data
     * -------------------------
     */
    dst->game       = src->game;
    dst->keeper = src->keeper;
    dst->buy_type = src->buy_type;
    dst->profit_buy = src->profit_buy;
    dst->profit_sell = src->profit_sell;
    dst->shop_type = src->shop_type;
    dst->profit_fix = src->profit_fix;
    dst->open_hour = src->open_hour;
    dst->close_hour = src->close_hour;

    return dst;
}

void olc_shop_free(SHOP_DATA* shop)
{
    if (!shop)
        return;

    DISPOSE(shop);
}

void olc_shop_apply_instance_changes(SHOP_DATA* dst, const SHOP_DATA* src)
{
    if (!dst || !src)
        return;

    dst->game       = src->game;
    dst->buy_type = src->buy_type;
    dst->profit_buy = src->profit_buy;
    dst->profit_sell = src->profit_sell;
    dst->shop_type = src->shop_type;
    dst->profit_fix = src->profit_fix;
    dst->open_hour = src->open_hour;
    dst->close_hour = src->close_hour;
}

void olc_show_shop(CHAR_DATA* ch, SHOP_DATA* shop, bool show_help, int term_width)
{
    if (!ch || !shop)
        return;

    ch_printf(ch, "%s[Shop %d]%s %sSTypes: %s%s \n%sITypes: %s%s%s\n",
        OLC_COL_HEADER,
        shop->keeper,
        OLC_COL_RESET,
        OLC_COL_STRING,
        bitset_to_string(shop->shop_type, shop_types).c_str(),
         OLC_COL_RESET,
         OLC_COL_STRING,
         OLC_COL_STRING,         
         bitset_to_string(shop->buy_type, o_types).c_str(),
        OLC_COL_RESET
    );
    ch_printf(ch, "%s[Buy: %s%3d%s %sSell: %s%3d%s %sFix: %s%3d%s %sOpen: %s%3d%s %sClose: %s%3d%s\n",
         OLC_COL_STRING,
        OLC_COL_INT,
        shop->profit_buy,
        OLC_COL_RESET,
         OLC_COL_STRING,
        OLC_COL_INT,
        shop->profit_sell,
         OLC_COL_RESET,
         OLC_COL_STRING,
         OLC_COL_INT,         
         shop->profit_fix,
         OLC_COL_RESET,
         OLC_COL_STRING,
         OLC_COL_INT,         
            shop->open_hour,
         OLC_COL_RESET,
         OLC_COL_STRING,
         OLC_COL_INT,       
             shop->close_hour,
         OLC_COL_RESET
    );
}

// ------------------------
// CRAFT_OLC craft_olc OlcOps commands
// ------------------------

void* craft_olc_clone(const void* src)
{
    if ( !src )
        return nullptr;

    return new CraftRecipe( *static_cast<const CraftRecipe*>(src) );
}

void craft_olc_free_clone(void* obj)
{
    delete static_cast<CraftRecipe*>(obj);
}

void craft_olc_apply_changes(void* original, void* working)
{
    CraftRecipe *dst = static_cast<CraftRecipe*>(original);
    const CraftRecipe *src = static_cast<const CraftRecipe*>(working);

    if ( !dst || !src )
        return;

    *dst = *src;
}

void craft_olc_save(CHAR_DATA* ch, void* original)
{
    CraftRecipe *recipe = static_cast<CraftRecipe*>( original );

    if ( !craft_validate_recipe_report(ch, recipe, "Committed craft recipe") )
    {
        if ( ch )
            send_to_char("Craft recipes were NOT written to disk.\n", ch);
        return;
    }

    craft_save_all_recipes_for_olc();

    if ( ch )
        send_to_char("Craft recipes written to disk.\n", ch);
}

void craft_olc_after_commit(CHAR_DATA* ch, void* /*original*/, void* /*working*/)
{
    if ( ch )
        send_to_char("Craft recipe committed.\n", ch);
}

void craft_olc_after_revert(CHAR_DATA* ch, void* /*original*/, void* /*working*/)
{
    if ( ch )
        send_to_char("Craft recipe reverted to pre-edit state.\n", ch);
}

const OlcOps craft_olc_ops =
{
    craft_olc_clone,
    craft_olc_free_clone,
    craft_olc_apply_changes,
    craft_olc_save,
    craft_olc_after_commit,
    craft_olc_after_revert,
    olc_craft_edit_interpret,
    OlcInterpretStage::EARLY
};

// ------------------------
// OLC_CRAFT olc_craft craft related commands
// ------------------------

bool olc_craft_in_edit_mode(CHAR_DATA* ch)
{
    return ch && ch->desc && ch->desc->olc &&
           ch->desc->olc->schema &&
           ch->desc->olc->schema->name &&
           !str_cmp(ch->desc->olc->schema->name, "craft");
}

void olc_craft_edit_help(CHAR_DATA* ch)
{
    send_to_char("CEDIT commands:\n", ch);
    send_to_char("  show [field]\n", ch);
    send_to_char("  help/? [field]\n", ch);
    send_to_char("  validate\n", ch);    
    send_to_char("  commit\n", ch);
    send_to_char("  revert\n", ch);
    send_to_char("  stop save\n", ch);
    send_to_char("  stop abort\n", ch);
    send_to_char("    or just save/abort\n", ch);
    send_to_char("  <field> <value>\n", ch);
}

static void olc_craft_mark_dirty( CHAR_DATA *ch )
{
    if ( ch && ch->desc && ch->desc->olc )
        ch->desc->olc->dirty = true;
}


bool olc_craft_edit_revert(CHAR_DATA *ch)
{
    if (!ch || !ch->desc || !ch->desc->olc)
        return false;

    auto sess = ch->desc->olc;

    /* Must be editing a craft recipe */
    if (!sess->schema || !sess->schema->name ||
        str_cmp(sess->schema->name, "craft"))
        return false;

    if (!sess->original_clone || !sess->ops ||
        !sess->ops->clone || !sess->ops->free_clone)
    {
        send_to_char("No revert snapshot is available.\n", ch);
        return false;
    }

    /* Free current working copy */
    if (sess->working_copy)
        sess->ops->free_clone(sess->working_copy);

    /* Restore from original clone */
    sess->working_copy = sess->ops->clone(sess->original_clone);
    if (!sess->working_copy)
    {
        send_to_char("Failed to restore revert snapshot.\n", ch);
        return false;
    }

    sess->last_cmd_arg.clear();
    sess->dirty = false;

    if (sess->ops->after_revert)
        sess->ops->after_revert(ch, sess->original, sess->working_copy);

    send_to_char("Craft recipe reverted to pre-edit state.\n", ch);
    return true;
}

static bool olc_craft_is_field_name( const char *input )
{
    if ( !input || input[0] == '\0' )
        return false;

    const auto *schema = get_craft_schema();

    if ( olc_find_field_fuzzy(schema, input) )
        return true;

    if ( olc_field_name_is_ambiguous(schema, input) )
        return true;

    return false;
}

static bool olc_craft_edit_interpret( CHAR_DATA *ch, const std::string &command, const std::string &argument )
{
    if ( !olc_craft_in_edit_mode(ch) || command.empty() )
        return false;

    if ( olc_command_matches(command, "help") || !str_cmp(command, "?") )
    {
        if ( argument.empty() )
            olc_craft_edit_help(ch);
        else
            olc_show(ch, argument, "help");
        return true;
    }

    if ( olc_command_matches(command, "stop") )
    {
        std::string arg = argument;
        std::string mode;
        arg = one_argument(arg, mode);

        if ( !str_cmp(mode, "save") )
        {
            olc_stop(ch, true);
            return true;
        }

        if ( !str_cmp(mode, "abort") )
        {
            olc_stop(ch, false);
            return true;
        }

        send_to_char("Syntax: stop save|abort\n", ch);
        return true;
    }

    if ( olc_command_matches(command, "abort") || olc_command_matches(command, "cancel") )
    {
        olc_stop(ch, false);
        return true;
    }

    if ( olc_command_matches(command, "save") || olc_command_matches(command, "done") )
    {
        olc_stop(ch, true);
        return true;
    }

    if ( olc_command_matches(command, "revert") )
    {
        if (olc_command_matches(command, "revert"))
        {
            olc_craft_edit_revert(ch);
            return true;
        }
    }

    if ( olc_command_matches(command, "commit") )
    {
        if ( olc_commit_current(ch) )
            send_to_char("Current changes committed.\n", ch);
        else
            send_to_char("No pending changes to commit.\n", ch);
        return true;
    }

    if ( olc_command_matches(command, "show") || olc_command_matches(command, "olcshow") )
    {
        do_olcshow(ch, const_cast<char*>(argument.c_str()));
        return true;
    }

    if ( olc_command_matches(command, "olcset") )
    {
        do_olcset(ch, const_cast<char*>(argument.c_str()));
        return true;
    }

    if (olc_command_matches(command, "validate") || olc_command_matches(command, "check"))
    {
        CraftRecipe *working = olc_session_working_as<CraftRecipe>( ch->desc->olc );
        craft_validate_recipe_report(ch, working, "Working craft recipe");
        return true;
    }

    if ( olc_craft_is_field_name(command.c_str()) )
    {
        std::string buf = command;
        if ( !argument.empty() )
        {
            buf += " ";
            buf += argument;
        }

        do_olcset(ch, const_cast<char*>(buf.c_str()));
        return true;
    }

    return false;
}

static bool craft_is_valid_result_target( const std::string &target )
{
    if ( target.empty() )
        return false;

    return !str_cmp(target, "value0")
        || !str_cmp(target, "value1")
        || !str_cmp(target, "value2")
        || !str_cmp(target, "value3")
        || !str_cmp(target, "value4")
        || !str_cmp(target, "value5")
        || !str_cmp(target, "weight")
        || !str_cmp(target, "cost")
        || !str_cmp(target, "level");
}

static void craft_collect_expr_var_refs( const CraftExpr &expr, std::set<std::string> &out )
{
    if ( !str_cmp(expr.op, "var") && !expr.var_name.empty() )
        out.insert( expr.var_name );

    if ( expr.left )
        craft_collect_expr_var_refs( *expr.left, out );

    if ( expr.right )
        craft_collect_expr_var_refs( *expr.right, out );

    if ( expr.third )
        craft_collect_expr_var_refs( *expr.third, out );

    if ( expr.fourth )
        craft_collect_expr_var_refs( *expr.fourth, out );        
}

static bool craft_validate_expr_vars( const CraftExpr &expr, const std::set<std::string> &valid_vars, std::string &err )
{
    std::set<std::string> refs;
    craft_collect_expr_var_refs( expr, refs );

    for ( const auto &name : refs )
    {
        if ( valid_vars.find(name) == valid_vars.end() )
        {
            err = str_printf( "Unknown var() reference: %s", name.c_str() );
            return false;
        }
    }

    return true;
}

static bool craft_validate_recipe( const CraftRecipe *recipe, std::vector<std::string> &errors )
{
    if ( !recipe )
    {
        errors.push_back( "Recipe pointer was null." );
        return false;
    }

    std::set<std::string> slot_names;
    std::set<std::string> derived_var_names;
    std::set<std::string> valid_vars;
    std::set<std::string> assignment_targets;
    bool has_base_slot = false;

    if ( recipe->name.empty() )
        errors.push_back( "Recipe name is empty." );

    if ( recipe->display_name.empty() )
        errors.push_back( "Recipe display name is empty." );

    if ( recipe->gsn < -1 )
        errors.push_back( "Recipe gsn is less than -1." );

    if ( recipe->timer_ticks < 0 )
        errors.push_back( "Recipe timer is negative." );

    if ( recipe->requires_extra_arg && recipe->extra_arg_name.empty() )
        errors.push_back( "Recipe requires extra arg but extra_arg_name is empty." );

    if ( recipe->output_mode == CraftOutputMode::CreateFromPrototype )
    {
        if ( recipe->result_vnum <= 0 )
            errors.push_back( "Create-prototype recipe requires result_vnum > 0." );
    }
    else if ( recipe->output_mode == CraftOutputMode::TransformBaseMaterial )
    {
        for ( const auto &slot : recipe->slots )
            if ( slot.may_be_base_material )
                has_base_slot = true;

        if ( !has_base_slot )
            errors.push_back( "Transform-base recipe has no slot marked may_be_base_material." );
    }

    if ( recipe->result_item_type < 0 )
        errors.push_back( "Result item type is invalid." );

    for ( size_t i = 0; i < recipe->slots.size(); ++i )
    {
        const CraftSlotDef &slot = recipe->slots[i];

        if ( slot.name.empty() )
            errors.push_back( str_printf("Slot #%zu has an empty name.", i + 1) );

        if ( !slot.name.empty() )
        {
            if ( slot_names.find(slot.name) != slot_names.end() )
                errors.push_back( str_printf("Duplicate slot name: %s", slot.name.c_str()) );
            else
                slot_names.insert( slot.name );
        }

        if ( slot.item_type < 0 )
            errors.push_back( str_printf("Slot '%s' has invalid item type.", slot.name.c_str()) );

        if ( slot.min_count < 0 )
            errors.push_back( str_printf("Slot '%s' has min_count < 0.", slot.name.c_str()) );

        if ( slot.max_count < 0 )
            errors.push_back( str_printf("Slot '%s' has max_count < 0.", slot.name.c_str()) );

        if ( slot.max_count > 0 && slot.min_count > slot.max_count )
            errors.push_back( str_printf("Slot '%s' has min_count greater than max_count.", slot.name.c_str()) );
    }

    for ( const auto &builtin : {
        std::string("base_value0"), std::string("base_value1"), std::string("base_value2"),
        std::string("base_value3"), std::string("base_value4"), std::string("base_value5"),
        std::string("base_weight"), std::string("base_cost"), std::string("base_level"),
        std::string("race"), std::string("sex")
    } )
    {
        valid_vars.insert( builtin );
    }

    for ( size_t i = 0; i < recipe->derived_vars.size(); ++i )
    {
        const CraftDerivedVarDef &dv = recipe->derived_vars[i];

        if ( dv.var_name.empty() )
            errors.push_back( str_printf("Derived var #%zu has an empty var_name.", i + 1) );

        if ( !dv.var_name.empty() )
        {
            if ( derived_var_names.find(dv.var_name) != derived_var_names.end() )
                errors.push_back( str_printf("Duplicate derived var name: %s", dv.var_name.c_str()) );
            else
            {
                derived_var_names.insert( dv.var_name );
                valid_vars.insert( dv.var_name );
            }
        }

        if ( dv.slot_name.empty() || slot_names.find(dv.slot_name) == slot_names.end() )
            errors.push_back( str_printf("Derived var '%s' references missing slot '%s'.",
                dv.var_name.c_str(), dv.slot_name.c_str()) );

        if ( !craft_is_valid_derived_source_field(dv.source_field) )
            errors.push_back( str_printf("Derived var '%s' has invalid source field '%s'.",
                dv.var_name.c_str(), dv.source_field.c_str()) );

        if ( !craft_is_valid_derived_aggregate_op(dv.aggregate_op) )
            errors.push_back( str_printf("Derived var '%s' has invalid aggregate op '%s'.",
                dv.var_name.c_str(), dv.aggregate_op.c_str()) );

        if ( dv.clamp_max_from_skill_mult && dv.clamp_max_constant )
            errors.push_back( str_printf("Derived var '%s' enables both skill max and constant max.",
                dv.var_name.c_str()) );

        if ( dv.clamp_max_from_skill_mult && dv.clamp_max_skill_mult < 0 )
            errors.push_back( str_printf("Derived var '%s' has negative skill multiplier.",
                dv.var_name.c_str()) );
    }

    for ( size_t i = 0; i < recipe->assignments.size(); ++i )
    {
        const CraftAssignment &asgn = recipe->assignments[i];
        std::string err;

        if ( !craft_is_valid_result_target(asgn.target) )
            errors.push_back( str_printf("Assignment #%zu has invalid target '%s'.",
                i + 1, asgn.target.c_str()) );
        else
        {
            if ( assignment_targets.find(asgn.target) != assignment_targets.end() )
                errors.push_back( str_printf("Duplicate assignment target: %s", asgn.target.c_str()) );
            else
                assignment_targets.insert( asgn.target );
        }

        if ( !craft_validate_expr_vars(asgn.expr, valid_vars, err) )
            errors.push_back( str_printf("Assignment '%s': %s",
                asgn.target.c_str(), err.c_str()) );
    }

    for ( size_t i = 0; i < recipe->affects.size(); ++i )
    {
        const CraftAffectDef &aff = recipe->affects[i];
        std::string err;

        if ( aff.location < 0 || !a_types[aff.location].name )
            errors.push_back( str_printf("Affect #%zu has invalid location %d.",
                i + 1, aff.location) );

        if ( aff.bitvector >= 0 && !find_flag(aff_flags, craft_affect_bitvector_name(aff.bitvector)) )
            errors.push_back( str_printf("Affect #%zu has invalid bitvector %d.",
                i + 1, aff.bitvector) );

        if ( !craft_validate_expr_vars(aff.modifier_expr, valid_vars, err) )
            errors.push_back( str_printf("Affect #%zu modifier: %s", i + 1, err.c_str()) );

        if ( aff.use_duration )
        {
            if ( !craft_validate_expr_vars(aff.duration_expr, valid_vars, err) )
                errors.push_back( str_printf("Affect #%zu duration: %s", i + 1, err.c_str()) );
        }
    }

    return errors.empty();
}

static bool craft_validate_recipe_report( CHAR_DATA *ch, const CraftRecipe *recipe, const char *label )
{
    std::vector<std::string> errors;

    if ( !ch )
        return false;

    if ( craft_validate_recipe(recipe, errors) )
    {
        ch_printf(ch, "%s is valid.\n", label ? label : "Recipe");
        return true;
    }

    ch_printf(ch, "%s is invalid:\n", label ? label : "Recipe");
    for ( const auto &err : errors )
        ch_printf(ch, "  - %s\n", err.c_str());

    return false;
}

static std::string craft_affect_location_name( int location )
{
    if ( location >= 0 && location < MAX_APPLY_TYPE && a_types[location].name )
        return a_types[location].name;

    return std::to_string(location);
}

static std::string craft_affect_bitvector_name( int bitvector )
{
    if ( bitvector < 0 )
        return "none";

    return get_enum_flag_field( bitvector, aff_flags );
}

static std::string craft_affect_summary( const CraftRecipe *recipe )
{
    if ( !recipe || recipe->affects.empty() )
        return "none";

    std::string out;

    for ( size_t i = 0; i < recipe->affects.size(); ++i )
    {
        const CraftAffectDef &aff = recipe->affects[i];

        if ( !out.empty() )
            out += " ; \n";

        out += "#";
        out += std::to_string(i + 1);
        out += " ";
        out += craft_affect_location_name( aff.location );
        out += "=";
        out += craft_expr_to_string_for_olc( aff.modifier_expr );

        if ( aff.bitvector >= 0 )
        {
            out += " bit=";
            out += craft_affect_bitvector_name( aff.bitvector );
        }

        if ( aff.use_duration )
        {
            out += " dur=";
            out += craft_expr_to_string_for_olc( aff.duration_expr );
        }
    }

    return out;
}

static void olc_craft_show_affect_help( CHAR_DATA *ch )
{
    send_to_char("AFFECTS commands:\n", ch);
    send_to_char("  affects\n", ch);
    send_to_char("  affects ?\n", ch);
    send_to_char("  affects add <location> <modifier-expr>\n", ch);
    send_to_char("  affects set <#> location <location>\n", ch);
    send_to_char("  affects set <#> modifier <expr>\n", ch);
    send_to_char("  affects set <#> bitvector <flag|none>\n", ch);
    send_to_char("  affects set <#> useduration <true|false>\n", ch);
    send_to_char("  affects set <#> duration <expr>\n", ch);
    send_to_char("  affects delete <#>\n", ch);
    send_to_char("\n", ch);
    send_to_char("Affect locations are the normal APPLY_* names used by get_atype.\n", ch);
    send_to_char("Bitvector uses aff_flags, or 'none'.\n", ch);
    send_to_char("Modifier and duration use normal craft expressions.\n", ch);
}

static int olc_craft_find_affect_index( const CraftRecipe *recipe, const std::string &which )
{
    if ( !recipe || which.empty() )
        return -1;

    try
    {
        int n = std::stoi(which);
        if ( n >= 1 && n <= (int)recipe->affects.size() )
            return n - 1;
    }
    catch (...)
    {
    }

    return -1;
}

static void olc_craft_show_affect_location_choices( CHAR_DATA *ch )
{
    if ( !ch )
        return;

    send_to_char("Valid affect locations:\n", ch);

    for ( int i = 0; i < MAX_APPLY_TYPE; ++i )
    {
        if ( !a_types[i].name || a_types[i].name[0] == '\0' )
            continue;

        ch_printf(ch, "  %-2d %s\n", i, a_types[i].name);
    }
}

static bool olc_craft_edit_affects_field( CHAR_DATA *ch, CraftRecipe *recipe )
{
    if ( !ch || !ch->desc || !ch->desc->olc || !recipe )
        return false;

    std::string input = ch->desc->olc->last_cmd_arg;
    std::string command;
    std::string rest;

    rest = one_argument(input, command);

    if ( command.empty() || !str_cmp(command, "?") || !str_cmp(command, "help") )
    {
        olc_craft_show_affect_help(ch);
        return false;
    }

    if ( !str_cmp(command, "list") || !str_cmp(command, "show") )
    {
        if ( recipe->affects.empty() )
        {
            send_to_char("No affects defined.\n", ch);
            return true;
        }

        for ( size_t i = 0; i < recipe->affects.size(); ++i )
        {
            const CraftAffectDef &aff = recipe->affects[i];

            ch_printf(ch,
                "#%zu loc=%s mod=%s bit=%s usedur=%s dur=%s\n",
                i + 1,
                craft_affect_location_name( aff.location ).c_str(),
                craft_expr_to_string_for_olc( aff.modifier_expr ).c_str(),
                craft_affect_bitvector_name( aff.bitvector ).c_str(),
                aff.use_duration ? "true" : "false",
                aff.use_duration
                    ? craft_expr_to_string_for_olc( aff.duration_expr ).c_str()
                    : "none");
        }

        return true;
    }

    if ( !str_cmp(command, "add") )
    {
        std::string location_name;
        std::string expr_text;
        int location;
        CraftExpr expr;
        std::string err;

        rest = one_argument(rest, location_name);
        expr_text = rest;

        if ( location_name.empty() || expr_text.empty() )
        {
            send_to_char("Syntax: affects add <location> <modifier-expr>\n", ch);
            return false;
        }

        location = get_atype( location_name );
        if ( location < 1 )
        {
            ch_printf(ch, "Unknown affect location: %s\n", location_name.c_str() );
            olc_craft_show_affect_location_choices(ch);
            return false;
        }

        if ( !craft_parse_expr_for_olc(expr_text, expr, err) )
        {
            ch_printf(ch, "Invalid modifier expression: %s\n", err.c_str() );
            return false;
        }

        CraftAffectDef aff;
        aff.location = location;
        aff.modifier_expr = std::move(expr);
        aff.bitvector = -1;
        aff.use_duration = false;

        recipe->affects.push_back( std::move(aff) );
        send_to_char("Affect added.\n", ch);
        return true;
    }

    if ( !str_cmp(command, "delete") || !str_cmp(command, "remove") )
    {
        std::string which;
        int idx;

        rest = one_argument(rest, which);

        if ( which.empty() )
        {
            send_to_char("Syntax: affects delete <#>\n", ch);
            return false;
        }

        idx = olc_craft_find_affect_index(recipe, which);
        if ( idx < 0 )
        {
            send_to_char("Affect not found.\n", ch);
            return false;
        }

        recipe->affects.erase( recipe->affects.begin() + idx );
        send_to_char("Affect deleted.\n", ch);
        return true;
    }

    if ( !str_cmp(command, "set") )
    {
        std::string which, field, value;
        int idx;

        rest = one_argument(rest, which);
        rest = one_argument(rest, field);
        value = rest;

        if ( which.empty() || field.empty() || value.empty() )
        {
            send_to_char("Syntax: affects set <#> <field> <value>\n", ch);
            return false;
        }

        idx = olc_craft_find_affect_index(recipe, which);
        if ( idx < 0 )
        {
            send_to_char("Affect not found.\n", ch);
            return false;
        }

        CraftAffectDef &aff = recipe->affects[idx];

        if ( !str_cmp(field, "location") )
        {
            int location = get_atype( value );
            if ( location < 1 )
            {
                ch_printf(ch, "Unknown affect location: %s\n", value.c_str() );
                olc_craft_show_affect_location_choices(ch);
                return false;
            }

            aff.location = location;
            send_to_char("Affect location updated.\n", ch);
            return true;
        }

        if ( !str_cmp(field, "modifier") )
        {
            CraftExpr expr;
            std::string err;

            if ( !craft_parse_expr_for_olc(value, expr, err) )
            {
                ch_printf(ch, "Invalid modifier expression: %s\n", err.c_str() );
                return false;
            }

            aff.modifier_expr = std::move(expr);
            send_to_char("Affect modifier updated.\n", ch);
            return true;
        }

        if ( !str_cmp(field, "bitvector") || !str_cmp(field, "bit") )
        {
            if ( !str_cmp(value, "none") )
            {
                aff.bitvector = -1;
                send_to_char("Affect bitvector cleared.\n", ch);
                return true;
            }

            const flag_name *flag = find_flag( aff_flags, value );
            if ( !flag )
            {
                send_to_char("Unknown affect bitvector.\n", ch);
                return false;
            }

            aff.bitvector = flag->bit;
            send_to_char("Affect bitvector updated.\n", ch);
            return true;
        }

        if ( !str_cmp(field, "useduration") || !str_cmp(field, "durationon") )
        {
            bool b;
            if ( !set_bool_field(b, value) )
            {
                send_to_char("useduration must be true/false.\n", ch);
                return false;
            }

            aff.use_duration = b;
            send_to_char("Affect use_duration updated.\n", ch);
            return true;
        }

        if ( !str_cmp(field, "duration") )
        {
            CraftExpr expr;
            std::string err;

            if ( !craft_parse_expr_for_olc(value, expr, err) )
            {
                ch_printf(ch, "Invalid duration expression: %s\n", err.c_str() );
                return false;
            }

            aff.duration_expr = std::move(expr);
            send_to_char("Affect duration expression updated.\n", ch);
            return true;
        }

        send_to_char("Unknown affect field. Use: location modifier bitvector useduration duration\n", ch);
        return false;
    }

    olc_craft_show_affect_help(ch);
    return false;
}

static bool craft_is_valid_assignment_target( const std::string &target )
{
    if ( target.empty() )
        return false;

    return !str_cmp(target, "value0")
        || !str_cmp(target, "value1")
        || !str_cmp(target, "value2")
        || !str_cmp(target, "value3")
        || !str_cmp(target, "value4")
        || !str_cmp(target, "value5")
        || !str_cmp(target, "weight")
        || !str_cmp(target, "cost")
        || !str_cmp(target, "level");
}

static std::string craft_assignment_summary( const CraftRecipe *recipe )
{
    if ( !recipe || recipe->assignments.empty() )
        return "none";

    std::string out;

    for ( size_t i = 0; i < recipe->assignments.size(); ++i )
    {
        const CraftAssignment &asgn = recipe->assignments[i];

        if ( !out.empty() )
            out += " ; ";

        out += "#";
        out += std::to_string(i + 1);
        out += " ";
        out += asgn.target;
        out += "=";
        out += craft_expr_to_string_for_olc( asgn.expr );
    }

    return out;
}

static void olc_craft_show_assignment_help( CHAR_DATA *ch )
{
    send_to_char("ASSIGNMENTS commands:\n", ch);
    send_to_char("  assignments\n", ch);
    send_to_char("  assignments ?\n", ch);
    send_to_char("  assignments add <target> <expr>\n", ch);
    send_to_char("  assignments set <target/#> target <newtarget>\n", ch);
    send_to_char("  assignments set <target/#> expr <expression>\n", ch);
    send_to_char("  assignments delete <target/#>\n", ch);
    send_to_char("\n", ch);
    send_to_char("Valid targets:\n", ch);
    send_to_char("  value0 value1 value2 value3 value4 value5 weight cost level\n", ch);
    send_to_char("\n", ch);
    send_to_char("Expression forms:\n", ch);
    send_to_char("  const(5)\n", ch);
    send_to_char("  skill\n", ch);
    send_to_char("  var(base_value1)\n", ch);
    send_to_char("  add(expr,expr)\n", ch);
    send_to_char("  sub(expr,expr)\n", ch);
    send_to_char("  mul(expr,expr)\n", ch);
    send_to_char("  div(expr,expr)\n", ch);
    send_to_char("  min(expr,expr)\n", ch);
    send_to_char("  max(expr,expr)\n", ch);
    send_to_char("  iflt(a,b,then,else) - if a < b then then else else\n", ch);    
}

static int olc_craft_find_assignment_index( const CraftRecipe *recipe, const std::string &which )
{
    if ( !recipe || which.empty() )
        return -1;

    try
    {
        int n = std::stoi(which);
        if ( n >= 1 && n <= (int)recipe->assignments.size() )
            return n - 1;
    }
    catch (...)
    {
    }

    for ( size_t i = 0; i < recipe->assignments.size(); ++i )
    {
        if ( !str_cmp(which, recipe->assignments[i].target) )
            return (int)i;
    }

    for ( size_t i = 0; i < recipe->assignments.size(); ++i )
    {
        if ( !str_prefix(which, recipe->assignments[i].target) )
            return (int)i;
    }

    return -1;
}

static bool olc_craft_edit_assignments_field( CHAR_DATA *ch, CraftRecipe *recipe )
{
    if ( !ch || !ch->desc || !ch->desc->olc || !recipe )
        return false;

    std::string input = ch->desc->olc->last_cmd_arg;
    std::string command;
    std::string rest;

    rest = one_argument(input, command);

    if ( command.empty() || !str_cmp(command, "?") || !str_cmp(command, "help") )
    {
        olc_craft_show_assignment_help(ch);
        return false;
    }

    if ( !str_cmp(command, "list") || !str_cmp(command, "show") )
    {
        if ( recipe->assignments.empty() )
        {
            send_to_char("No assignments defined.\n", ch);
            return true;
        }

        for ( size_t i = 0; i < recipe->assignments.size(); ++i )
        {
            const CraftAssignment &asgn = recipe->assignments[i];

            ch_printf(ch, "#%zu %-8s = %s\n",
                i + 1,
                asgn.target.c_str(),
                craft_expr_to_string_for_olc( asgn.expr ).c_str() );
        }

        return true;
    }

    if ( !str_cmp(command, "add") )
    {
        std::string target;
        std::string expr_text;
        CraftExpr expr;
        std::string err;

        rest = one_argument(rest, target);
        expr_text = rest;

        if ( target.empty() || expr_text.empty() )
        {
            send_to_char("Syntax: assignments add <target> <expr>\n", ch);
            return false;
        }

        if ( !craft_is_valid_assignment_target(target) )
        {
            send_to_char("Invalid assignment target.\n", ch);
            return false;
        }

        if ( olc_craft_find_assignment_index(recipe, target) >= 0 )
        {
            send_to_char("An assignment for that target already exists.\n", ch);
            return false;
        }

        if ( !craft_parse_expr_for_olc(expr_text, expr, err) )
        {
            ch_printf(ch, "Invalid expression: %s\n", err.c_str() );
            return false;
        }

        CraftAssignment asgn;
        asgn.target = target;
        asgn.expr = std::move(expr);

        recipe->assignments.push_back( std::move(asgn) );
        send_to_char("Assignment added.\n", ch);
        olc_craft_mark_dirty(ch);
        return true;
    }

    if ( !str_cmp(command, "delete") || !str_cmp(command, "remove") )
    {
        std::string which;
        int idx;

        rest = one_argument(rest, which);

        if ( which.empty() )
        {
            send_to_char("Syntax: assignments delete <target/#>\n", ch);
            return false;
        }

        idx = olc_craft_find_assignment_index(recipe, which);
        if ( idx < 0 )
        {
            send_to_char("Assignment not found.\n", ch);
            return false;
        }

        recipe->assignments.erase( recipe->assignments.begin() + idx );
        send_to_char("Assignment deleted.\n", ch);
        olc_craft_mark_dirty(ch);

        return true;
    }

    if ( !str_cmp(command, "set") )
    {
        std::string which, field, value;
        int idx;

        rest = one_argument(rest, which);
        rest = one_argument(rest, field);
        value = rest;

        if ( which.empty() || field.empty() || value.empty() )
        {
            send_to_char("Syntax: assignments set <target/#> <field> <value>\n", ch);
            return false;
        }

        idx = olc_craft_find_assignment_index(recipe, which);
        if ( idx < 0 )
        {
            send_to_char("Assignment not found.\n", ch);
            return false;
        }

        CraftAssignment &asgn = recipe->assignments[idx];

        if ( !str_cmp(field, "target") )
        {
            if ( !craft_is_valid_assignment_target(value) )
            {
                send_to_char("Invalid assignment target.\n", ch);
                return false;
            }

            if ( olc_craft_find_assignment_index(recipe, value) >= 0 && str_cmp(asgn.target, value) )
            {
                send_to_char("An assignment for that target already exists.\n", ch);
                return false;
            }

            asgn.target = value;
            send_to_char("Assignment target updated.\n", ch);
            olc_craft_mark_dirty(ch);
            return true;
        }

        if ( !str_cmp(field, "expr") || !str_cmp(field, "expression") )
        {
            CraftExpr expr;
            std::string err;

            if ( !craft_parse_expr_for_olc(value, expr, err) )
            {
                ch_printf(ch, "Invalid expression: %s\n", err.c_str() );
                return false;
            }

            asgn.expr = std::move(expr);
            send_to_char("Assignment expression updated.\n", ch);
            olc_craft_mark_dirty(ch);
            return true;
        }

        send_to_char("Unknown assignment field. Fields: target, expr\n", ch);
        return false;
    }

    olc_craft_show_assignment_help(ch);
    return false;
}

static bool craft_is_valid_derived_source_field( const std::string &field )
{
    if ( field.empty() )
        return false;

    return !str_cmp(field, "value0")
        || !str_cmp(field, "value1")
        || !str_cmp(field, "value2")
        || !str_cmp(field, "value3")
        || !str_cmp(field, "value4")
        || !str_cmp(field, "value5")
        || !str_cmp(field, "weight")
        || !str_cmp(field, "cost")
        || !str_cmp(field, "level");
}

static bool craft_is_valid_derived_aggregate_op( const std::string &op )
{
    if ( op.empty() )
        return false;

    return !str_cmp(op, "count")
        || !str_cmp(op, "first")
        || !str_cmp(op, "highest")
        || !str_cmp(op, "lowest")
        || !str_cmp(op, "sum");
}

static std::string craft_derived_var_summary( const CraftRecipe *recipe )
{
    if ( !recipe || recipe->derived_vars.empty() )
        return "none";

    std::string out;

    for ( size_t i = 0; i < recipe->derived_vars.size(); ++i )
    {
        const CraftDerivedVarDef &dv = recipe->derived_vars[i];

        if ( !out.empty() )
            out += " ; ";

        out += "#";
        out += std::to_string(i + 1);
        out += " ";
        out += dv.var_name;
        out += "=(";
        out += dv.slot_name;
        out += ".";
        out += dv.source_field;
        out += " ";
        out += dv.aggregate_op;
        out += ")";

        if ( dv.use_clamp )
        {
            out += " clamp[min=";
            out += std::to_string(dv.clamp_min);

            if ( dv.clamp_max_from_skill_mult )
            {
                out += ", max=skill*";
                out += std::to_string(dv.clamp_max_skill_mult);
            }
            else if ( dv.clamp_max_constant )
            {
                out += ", max=";
                out += std::to_string(dv.clamp_max_value);
            }
            else
            {
                out += ", max=result";
            }

            out += "]";
        }
    }

    return out;
}

static void olc_craft_show_derived_var_help( CHAR_DATA *ch )
{
    send_to_char("DERIVEDVARS commands:\n", ch);
    send_to_char("  derivedvar\n", ch);
    send_to_char("  derivedvar ?\n", ch);
    send_to_char("  derivedvar add <var> <slot> <source> <op>\n", ch);
    send_to_char("  derivedvar set <var/#> <field> <value>\n", ch);
    send_to_char("  derivedvar delete <var/#>\n", ch);
    send_to_char("\n", ch);
    send_to_char("Fields for 'derivedvar set' are:\n", ch);
    send_to_char("  var      slot      source      op\n", ch);
    send_to_char("  useclamp clampmin  maxskill    skillmult\n", ch);
    send_to_char("  maxconst maxvalue\n", ch);
    send_to_char("\n", ch);
    send_to_char("Valid source fields:\n", ch);
    send_to_char("  value0 value1 value2 value3 value4 value5 weight cost level\n", ch);
    send_to_char("Valid aggregate ops:\n", ch);
    send_to_char("  count first highest lowest sum\n", ch);
    send_to_char("\n", ch);
    send_to_char("Clamp rules:\n", ch);
    send_to_char("  useclamp true/false enables clamping.\n", ch);
    send_to_char("  clampmin is the lower bound.\n", ch);
    send_to_char("  maxskill true means max = skill * skillmult.\n", ch);
    send_to_char("  maxconst true means max = maxvalue.\n", ch);
    send_to_char("  If both maxskill and maxconst are false, max stays as the result value.\n", ch);
}

static int olc_craft_find_derived_var_index( const CraftRecipe *recipe, const std::string &which )
{
    if ( !recipe || which.empty() )
        return -1;

    try
    {
        int n = std::stoi(which);
        if ( n >= 1 && n <= (int)recipe->derived_vars.size() )
            return n - 1;
    }
    catch (...)
    {
    }

    for ( size_t i = 0; i < recipe->derived_vars.size(); ++i )
    {
        if ( !str_cmp(which, recipe->derived_vars[i].var_name) )
            return (int)i;
    }

    for ( size_t i = 0; i < recipe->derived_vars.size(); ++i )
    {
        if ( !str_prefix(which, recipe->derived_vars[i].var_name) )
            return (int)i;
    }

    return -1;
}

static bool olc_craft_edit_derived_vars_field( CHAR_DATA *ch, CraftRecipe *recipe )
{
    if ( !ch || !ch->desc || !ch->desc->olc || !recipe )
        return false;

    std::string input = ch->desc->olc->last_cmd_arg;
    std::string command;
    std::string rest;

    rest = one_argument(input, command);

    if ( command.empty() || !str_cmp(command, "?") || !str_cmp(command, "help") )
    {
        olc_craft_show_derived_var_help(ch);
        return false;
    }

    if ( !str_cmp(command, "list") || !str_cmp(command, "show") )
    {
        if ( recipe->derived_vars.empty() )
        {
            send_to_char("No derived vars defined.\n", ch);
            return true;
        }

        for ( size_t i = 0; i < recipe->derived_vars.size(); ++i )
        {
            const CraftDerivedVarDef &dv = recipe->derived_vars[i];

            ch_printf(ch,
                "#%zu %-16s slot=%-12s source=%-8s op=%-8s clamp=%s min=%d maxskill=%s skillmult=%d maxconst=%s maxvalue=%d\n",
                i + 1,
                dv.var_name.c_str(),
                dv.slot_name.c_str(),
                dv.source_field.c_str(),
                dv.aggregate_op.c_str(),
                dv.use_clamp ? "true" : "false",
                dv.clamp_min,
                dv.clamp_max_from_skill_mult ? "true" : "false",
                dv.clamp_max_skill_mult,
                dv.clamp_max_constant ? "true" : "false",
                dv.clamp_max_value);
        }

        return true;
    }

    if ( !str_cmp(command, "add") )
    {
        std::string var_name, slot_name, source_field, aggregate_op;

        rest = one_argument(rest, var_name);
        rest = one_argument(rest, slot_name);
        rest = one_argument(rest, source_field);
        rest = one_argument(rest, aggregate_op);

        if ( var_name.empty() || slot_name.empty() || source_field.empty() || aggregate_op.empty() )
        {
            send_to_char("Syntax: derivedvar add <var> <slot> <source> <op>\n", ch);
            return false;
        }

        if ( olc_craft_find_derived_var_index(recipe, var_name) >= 0 )
        {
            send_to_char("A derived var with that name already exists.\n", ch);
            return false;
        }

        if ( olc_craft_find_slot_index(recipe, slot_name) < 0 )
        {
            send_to_char("That slot does not exist.\n", ch);
            return false;
        }

        if ( !craft_is_valid_derived_source_field(source_field) )
        {
            send_to_char("Invalid source field.\n", ch);
            return false;
        }

        if ( !craft_is_valid_derived_aggregate_op(aggregate_op) )
        {
            send_to_char("Invalid aggregate op.\n", ch);
            return false;
        }

        CraftDerivedVarDef dv;
        dv.var_name = var_name;
        dv.slot_name = slot_name;
        dv.source_field = source_field;
        dv.aggregate_op = aggregate_op;

        recipe->derived_vars.push_back( std::move(dv) );
        send_to_char("Derived var added.\n", ch);
        olc_craft_mark_dirty(ch);
        return true;
    }

    if ( !str_cmp(command, "delete") || !str_cmp(command, "remove") )
    {
        std::string which;
        int idx;

        rest = one_argument(rest, which);

        if ( which.empty() )
        {
            send_to_char("Syntax: derivedvar delete <var/#>\n", ch);
            return false;
        }

        idx = olc_craft_find_derived_var_index(recipe, which);
        if ( idx < 0 )
        {
            send_to_char("Derived var not found.\n", ch);
            return false;
        }

        recipe->derived_vars.erase( recipe->derived_vars.begin() + idx );
        send_to_char("Derived var deleted.\n", ch);
        olc_craft_mark_dirty(ch);
        return true;
    }

    if ( !str_cmp(command, "set") )
    {
        std::string which, field, value;
        int idx;

        rest = one_argument(rest, which);
        rest = one_argument(rest, field);

        if ( which.empty() || field.empty() || rest.empty() )
        {
            send_to_char("Syntax: derivedvar set <var/#> <field> <value>\n", ch);
            return false;
        }

        value = rest;
        idx = olc_craft_find_derived_var_index(recipe, which);

        if ( idx < 0 )
        {
            send_to_char("Derived var not found.\n", ch);
            return false;
        }

        CraftDerivedVarDef &dv = recipe->derived_vars[idx];

        if ( !str_cmp(field, "var") || !str_cmp(field, "name") )
        {
            if ( olc_craft_find_derived_var_index(recipe, value) >= 0 && str_cmp(dv.var_name, value) )
            {
                send_to_char("A derived var with that name already exists.\n", ch);
                return false;
            }

            dv.var_name = value;
            send_to_char("Derived var name updated.\n", ch);
            return true;
        }

        if ( !str_cmp(field, "slot") )
        {
            if ( olc_craft_find_slot_index(recipe, value) < 0 )
            {
                send_to_char("That slot does not exist.\n", ch);
                return false;
            }

            dv.slot_name = value;
            send_to_char("Derived var slot updated.\n", ch);
            olc_craft_mark_dirty(ch);
            return true;
        }

        if ( !str_cmp(field, "source") || !str_cmp(field, "sourcefield") )
        {
            if ( !craft_is_valid_derived_source_field(value) )
            {
                send_to_char("Invalid source field.\n", ch);
                return false;
            }

            dv.source_field = value;
            send_to_char("Derived var source field updated.\n", ch);
            olc_craft_mark_dirty(ch);
            return true;
        }

        if ( !str_cmp(field, "op") || !str_cmp(field, "aggregate") )
        {
            if ( !craft_is_valid_derived_aggregate_op(value) )
            {
                send_to_char("Invalid aggregate op.\n", ch);
                return false;
            }

            dv.aggregate_op = value;
            send_to_char("Derived var aggregate op updated.\n", ch);
            olc_craft_mark_dirty(ch);
            return true;
        }

        if ( !str_cmp(field, "useclamp") || !str_cmp(field, "clamp") )
        {
            bool b;
            if ( !set_bool_field(b, value) )
            {
                send_to_char("useclamp must be true/false.\n", ch);
                return false;
            }

            dv.use_clamp = b;
            send_to_char("Derived var use_clamp updated.\n", ch);
            olc_craft_mark_dirty(ch);
            return true;
        }

        if ( !str_cmp(field, "clampmin") || !str_cmp(field, "min") )
        {
            try
            {
                dv.clamp_min = std::stoi(value);
                send_to_char("Derived var clamp_min updated.\n", ch);
                olc_craft_mark_dirty(ch);
                return true;
            }
            catch (...)
            {
                send_to_char("clampmin must be a number.\n", ch);
                return false;
            }
        }

        if ( !str_cmp(field, "maxskill") || !str_cmp(field, "skillmax") )
        {
            bool b;
            if ( !set_bool_field(b, value) )
            {
                send_to_char("maxskill must be true/false.\n", ch);
                return false;
            }

            dv.clamp_max_from_skill_mult = b;
            if ( b )
                dv.clamp_max_constant = false;

            send_to_char("Derived var max-skill mode updated.\n", ch);
            olc_craft_mark_dirty(ch);
            return true;
        }

        if ( !str_cmp(field, "skillmult") || !str_cmp(field, "maxskillmult") )
        {
            try
            {
                dv.clamp_max_skill_mult = std::stoi(value);
                send_to_char("Derived var skill multiplier updated.\n", ch);
                olc_craft_mark_dirty(ch);
                return true;
            }
            catch (...)
            {
                send_to_char("skillmult must be a number.\n", ch);
                return false;
            }
        }

        if ( !str_cmp(field, "maxconst") || !str_cmp(field, "constmax") )
        {
            bool b;
            if ( !set_bool_field(b, value) )
            {
                send_to_char("maxconst must be true/false.\n", ch);
                return false;
            }

            dv.clamp_max_constant = b;
            if ( b )
                dv.clamp_max_from_skill_mult = false;

            send_to_char("Derived var max-constant mode updated.\n", ch);
            olc_craft_mark_dirty(ch);
            return true;
        }

        if ( !str_cmp(field, "maxvalue") || !str_cmp(field, "constvalue") )
        {
            try
            {
                dv.clamp_max_value = std::stoi(value);
                send_to_char("Derived var max value updated.\n", ch);
                olc_craft_mark_dirty(ch);
                return true;
            }
            catch (...)
            {
                send_to_char("maxvalue must be a number.\n", ch);
                return false;
            }
        }

        send_to_char("Unknown derived var field.\n", ch);
        return false;
    }

    olc_craft_show_derived_var_help(ch);
    return false;
}

static const char *craft_consume_mode_name( CraftConsumeMode mode )
{
    switch ( mode )
    {
        default:
        case CraftConsumeMode::Keep:          return "keep";
        case CraftConsumeMode::Consume:       return "consume";
        case CraftConsumeMode::TransformBase: return "transform_base";
    }
}

static bool craft_consume_mode_from_string( const std::string &value, CraftConsumeMode &out )
{
    if ( !str_cmp(value, "keep") )
    {
        out = CraftConsumeMode::Keep;
        return true;
    }

    if ( !str_cmp(value, "consume") )
    {
        out = CraftConsumeMode::Consume;
        return true;
    }

    if ( !str_cmp(value, "transform_base") || !str_cmp(value, "transform") || !str_cmp(value, "base") )
    {
        out = CraftConsumeMode::TransformBase;
        return true;
    }

    return false;
}

static bool craft_item_type_from_string( const std::string &value, int &out )
{
    if ( value.empty() )
        return false;

    if ( enum_from_string_flag(value, out, o_types) )
        return true;

    try
    {
        out = std::stoi(value);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

static std::string craft_slot_summary( const CraftRecipe *recipe )
{
    if ( !recipe || recipe->slots.empty() )
        return "none";

    std::string out;

    for ( size_t i = 0; i < recipe->slots.size(); ++i )
    {
        const CraftSlotDef &slot = recipe->slots[i];

        if ( !out.empty() )
            out += " ; ";

        out += "#";
        out += std::to_string(i + 1);
        out += " ";
        out += slot.name;
        out += " ";
        out += "(";
        out += get_enum_flag_field(slot.item_type, o_types);
        out += ", ";
        out += std::to_string(slot.min_count);
        out += "..";
        out += (slot.max_count > 0) ? std::to_string(slot.max_count) : std::string("inf");
        out += ", ";
        out += slot.min_count == 0 ? "optional" : "required";
        out += ", ";
        out += craft_consume_mode_name(slot.consume_mode);
        out += ", ";
        out += slot.may_be_base_material ? "base_ok" : "not_base";
        out += ")";
    }

    return out;
}

static void olc_craft_show_slot_help( CHAR_DATA *ch )
{
    send_to_char("SLOTS commands:\n", ch);
    send_to_char("  slots                       - show slot list\n", ch);
    send_to_char("  slots ?                     - show this help\n", ch);
    send_to_char("  slots add <name> <type> <min> <max> <consume> <base>\n", ch);
    send_to_char("  slots set <slot> <field> <value>\n", ch);
    send_to_char("  slots delete <slot>\n", ch);
    send_to_char("\n", ch);
    send_to_char("  <slot> may be a slot number or slot name.\n", ch);
    send_to_char("  <type> may be an object type name or raw number.\n", ch);
    send_to_char("  <consume> is keep, consume, or transform_base.\n", ch);
    send_to_char("  <base> is true/false.\n", ch);
    send_to_char("  <max> may be 0 for unlimited.\n", ch);
    send_to_char("\n", ch);
    send_to_char("Fields for 'slots set' are:\n", ch);
    send_to_char("  name  type  min  max  consume  base\n", ch);
}

static int olc_craft_find_slot_index( const CraftRecipe *recipe, const std::string &which )
{
    if ( !recipe || which.empty() )
        return -1;

    try
    {
        int n = std::stoi(which);
        if ( n >= 1 && n <= (int)recipe->slots.size() )
            return n - 1;
    }
    catch (...)
    {
    }

    for ( size_t i = 0; i < recipe->slots.size(); ++i )
    {
        if ( !str_cmp(which, recipe->slots[i].name) )
            return (int)i;
    }

    for ( size_t i = 0; i < recipe->slots.size(); ++i )
    {
        if ( !str_prefix(which, recipe->slots[i].name) )
            return (int)i;
    }

    return -1;
}

static bool olc_craft_edit_slots_field( CHAR_DATA *ch, CraftRecipe *recipe )
{
    if ( !ch || !ch->desc || !ch->desc->olc || !recipe )
        return false;

    std::string arg = ch->desc->olc->last_cmd_arg;
    std::string cmd;
    std::string rest;

    one_argument(arg, cmd);
    rest = arg;

    if ( cmd.empty() || !str_cmp(cmd, "?") || !str_cmp(cmd, "help") )
    {
        olc_craft_show_slot_help(ch);
        return false;
    }
/*
    if ( !str_cmp(cmd, "slots") )
        one_argument(rest, cmd);
    else
        cmd = "show";
*/
    if ( cmd.empty() || !str_cmp(cmd, "show") || !str_cmp(cmd, "list") )
    {
        if ( recipe->slots.empty() )
        {
            send_to_char("No slots defined.\n", ch);
            return true;
        }

        for ( size_t i = 0; i < recipe->slots.size(); ++i )
        {
            const CraftSlotDef &slot = recipe->slots[i];

            ch_printf(ch,
                "#%zu %-12s type=%s min=%d max=%s optional=%s consume=%s base=%s\n",
                i + 1,
                slot.name.c_str(),
                get_enum_flag_field(slot.item_type, o_types).c_str(),
                slot.min_count,
                (slot.max_count > 0 ? std::to_string(slot.max_count).c_str() : "inf"),
                slot.min_count == 0 ? "optional" : "required",
                craft_consume_mode_name(slot.consume_mode),
                slot.may_be_base_material ? "true" : "false");
        }
        return true;
    }

    if ( !str_cmp(cmd, "add") )
    {
        std::string name, type_s, min_s, max_s, consume_s, base_s;
        int item_type, min_count, max_count;
        bool base_ok;
        CraftConsumeMode consume_mode;

//        one_argument(rest, cmd);          // consume "slots"
        rest = one_argument(rest, cmd);          // consume "add"
        rest = one_argument(rest, name);
        rest = one_argument(rest, type_s);
        rest = one_argument(rest, min_s);
        rest = one_argument(rest, max_s);
        rest = one_argument(rest, consume_s);
        rest = one_argument(rest, base_s);

        if ( name.empty() || type_s.empty() || min_s.empty() || max_s.empty()
          || consume_s.empty() || base_s.empty() )
        {
            send_to_char("Syntax: slots add <name> <type> <min> <max> <consume> <base>\n", ch);
            return false;
        }

        if ( olc_craft_find_slot_index(recipe, name) >= 0 )
        {
            send_to_char("A slot with that name already exists.\n", ch);
            return false;
        }

        if ( !craft_item_type_from_string(type_s, item_type) )
        {
            send_to_char("Unknown item type.\n", ch);
            return false;
        }

        try
        {
            min_count = std::stoi(min_s);
            max_count = std::stoi(max_s);
        }
        catch (...)
        {
            send_to_char("min and max must be numbers.\n", ch);
            return false;
        }

        if ( min_count < 0 || max_count < 0 )
        {
            send_to_char("min and max must be 0 or greater.\n", ch);
            return false;
        }

        if ( max_count > 0 && min_count > max_count )
        {
            send_to_char("min cannot exceed max.\n", ch);
            return false;
        }

        if ( !craft_consume_mode_from_string(consume_s, consume_mode) )
        {
            send_to_char("consume must be keep, consume, or transform_base.\n", ch);
            return false;
        }

        if ( !set_bool_field(base_ok, base_s) )
        {
            send_to_char("base must be true/false.\n", ch);
            return false;
        }

        recipe->slots.push_back( CraftSlotDef{ name, item_type, min_count, max_count, consume_mode, base_ok } );
        send_to_char("Slot added.\n", ch);
        olc_craft_mark_dirty(ch);
        return true;
    }

    if ( !str_cmp(cmd, "delete") || !str_cmp(cmd, "remove") )
    {
        std::string which;
        int idx;

        //one_argument(rest, cmd);          // consume "slots"
        rest = one_argument(rest, cmd);          // consume "delete/remove"
        rest = one_argument(rest, which);

        if ( which.empty() )
        {
            send_to_char("Syntax: slots delete <slot>\n", ch);
            return false;
        }

        idx = olc_craft_find_slot_index(recipe, which);
        if ( idx < 0 )
        {
            send_to_char("Slot not found.\n", ch);
            return false;
        }

        recipe->slots.erase( recipe->slots.begin() + idx );
        send_to_char("Slot deleted.\n", ch);
        olc_craft_mark_dirty(ch);
        return true;
    }

    if ( !str_cmp(cmd, "set") )
    {
        std::string which, field, value;
        int idx;

        //one_argument(rest, cmd);          // consume "slots"
        rest = one_argument(rest, cmd);          // consume "set"
        rest = one_argument(rest, which);
        rest = one_argument(rest, field);

        value = rest;

        if ( which.empty() || field.empty() || value.empty() )
        {
            send_to_char("Syntax: slots set <slot> <field> <value>\n", ch);
            return false;
        }

        idx = olc_craft_find_slot_index(recipe, which);
        if ( idx < 0 )
        {
            send_to_char("Slot not found.\n", ch);
            return false;
        }

        CraftSlotDef &slot = recipe->slots[idx];

        if ( !str_cmp(field, "name") )
        {
            if ( olc_craft_find_slot_index(recipe, value) >= 0 && str_cmp(slot.name, value) )
            {
                send_to_char("A slot with that name already exists.\n", ch);
                return false;
            }

            slot.name = value;
            send_to_char("Slot name updated.\n", ch);
            olc_craft_mark_dirty(ch);
            return true;
        }

        if ( !str_cmp(field, "type") )
        {
            int item_type;
            if ( !craft_item_type_from_string(value, item_type) )
            {
                send_to_char("Unknown item type.\n", ch);
                return false;
            }

            slot.item_type = item_type;
            send_to_char("Slot type updated.\n", ch);
            olc_craft_mark_dirty(ch);
            return true;
        }

        if ( !str_cmp(field, "min") )
        {
            try
            {
                int v = std::stoi(value);
                if ( v < 0 )
                    throw 1;
                if ( slot.max_count > 0 && v > slot.max_count )
                {
                    send_to_char("min cannot exceed max.\n", ch);
                    return false;
                }
                slot.min_count = v;
                send_to_char("Slot min updated.\n", ch);
                olc_craft_mark_dirty(ch);
                return true;
            }
            catch (...)
            {
                send_to_char("min must be 0 or greater.\n", ch);
                return false;
            }
        }

        if ( !str_cmp(field, "max") )
        {
            try
            {
                int v = std::stoi(value);
                if ( v < 0 )
                    throw 1;
                if ( v > 0 && v < slot.min_count )
                {
                    send_to_char("max cannot be less than min unless max is 0.\n", ch);
                    return false;
                }
                slot.max_count = v;
                send_to_char("Slot max updated.\n", ch);
                olc_craft_mark_dirty(ch);
                return true;
            }
            catch (...)
            {
                send_to_char("max must be 0 or greater.\n", ch);
                return false;
            }
        }

        if ( !str_cmp(field, "consume") )
        {
            CraftConsumeMode mode;
            if ( !craft_consume_mode_from_string(value, mode) )
            {
                send_to_char("consume must be keep, consume, or transform_base.\n", ch);
                return false;
            }

            slot.consume_mode = mode;
            send_to_char("Slot consume mode updated.\n", ch);
            olc_craft_mark_dirty(ch);
            return true;
        }

        if ( !str_cmp(field, "base") || !str_cmp(field, "maybebase") || !str_cmp(field, "baseok") )
        {
            bool v;
            if ( !set_bool_field(v, value) )
            {
                send_to_char("base must be true/false.\n", ch);
                return false;
            }

            slot.may_be_base_material = v;
            send_to_char("Slot base flag updated.\n", ch);
            olc_craft_mark_dirty(ch);
            return true;
        }

        send_to_char("Unknown slot field. Use: name type min max consume base\n", ch);
        return false;
    }

    olc_craft_show_slot_help(ch);
    return false;
}

// ------------------------
// DO_COMMANDS
// ------------------------

void do_cfedit(CHAR_DATA* ch, char* argument)
{
    std::string arg1;
    std::string arg2;
    std::string argstr = argument;

    if (!ch || !ch->desc)
        return;

    /*
     * Existing craft edit session commands
     */
    if (olc_craft_in_edit_mode(ch))
    {
        std::string rest = argstr;
        rest = one_argument(rest, arg1);
        rest = one_argument(rest, arg2);

        if (argstr.empty())
        {
            olc_craft_edit_help(ch);
            return;
        }

        if (!str_cmp(arg1, "validate") || !str_cmp(arg1, "check"))
        {
            CraftRecipe *working = olc_session_working_as<CraftRecipe>( ch->desc->olc );
            craft_validate_recipe_report(ch, working, "Working craft recipe");
            return;
        }

        if ( !str_cmp(arg1, "save")
          || !str_cmp(arg1, "done")
          || ( !str_cmp(arg1, "stop") && !str_cmp(arg2, "save") ) )
        {
            CraftRecipe *working = olc_session_working_as<CraftRecipe>( ch->desc->olc );
            if ( !craft_validate_recipe_report(ch, working, "Working craft recipe") )
            {
                send_to_char("Save blocked until the recipe validates cleanly.\n", ch);
                return;
            }
        }

        do_olc(ch, argument);
        return;
    }

    argstr = one_argument(argstr, arg1);

    /*
     * Start a new craft recipe edit session
     */
    if (arg1.empty())
    {
        send_to_char("Usage: cfedit <recipe>\n", ch);
        return;
    }

    CraftRecipe *recipe = craft_find_recipe_for_olc(arg1);
    if (!recipe)
    {
        send_to_char("Craft recipe not found.\n", ch);
        return;
    }

    if (ch->desc->olc)
    {
        send_to_char("You are already editing something.\n", ch);
        return;
    }

    olc_start(ch, recipe, get_craft_schema(), &craft_olc_ops);
    if (!ch->desc->olc)
        return;

    ch->desc->olc->mode = OlcEditMode::CRAFT_INLINE;
    send_to_char("Craft inline editing mode enabled.\n", ch);
    send_to_char("To exit, type stop save/abort, or just save/abort.\n", ch);

    olc_show(ch, "", "");
}

void do_redit2(CHAR_DATA* ch, char* argument)
{
    std::string arg;
    std::string argstr = argument;

    if (olc_room_in_edit_mode(ch))
    {
        if (argstr.empty())
        {
            olc_room_edit_help(ch);
            return;
        }

        do_olc(ch, argument);
        return;
    }

    argstr = one_argument(argstr, arg);

    if (arg.empty() || !str_cmp(arg, "start"))
    {
        olc_room_edit_enter(ch);
        return;
    }

    if (!str_cmp(arg, "help") || !str_cmp(arg, "?"))
    {
        if (!argstr.empty())
            olc_show(ch, argstr, "help");
        else
            olc_show(ch, arg, "");
        return;
    }

    if (!str_cmp(arg, "stop"))
    {
        std::string arg2;
        argstr = one_argument(argstr, arg2);

        if (!str_cmp(arg2, "save") || arg2.empty())
        {
            olc_stop(ch, true);
            return;
        }

        if (!str_cmp(arg2, "abort"))
        {
            olc_stop(ch, false);
            return;
        }
    }

    if (!str_cmp(arg, "abort") || !str_cmp(arg, "cancel"))
    {
        olc_stop(ch, false);
        return;
    }

    if (!str_cmp(arg, "save"))
    {
        olc_stop(ch, true);
        return;
    }

    send_to_char("Syntax: redit (start)\n", ch);
    send_to_char("        redit stop save|abort\n", ch);
    send_to_char("or just redit save|abort\n", ch);    
}

void do_oedit(CHAR_DATA* ch, char* argument)
{
    std::string arg1;
    std::string arg2;
    std::string argstr = argument;

    if (!ch || !ch->desc)
        return;

    /* =========================
     * Existing object edit session commands
     * ========================= */
    if (olc_object_in_edit_mode(ch))
    {
        if (argstr.empty())
        {
            olc_object_edit_help(ch);
            return;
        }
        do_olc(ch, argument);
        return;
    }

    argstr = one_argument(argstr, arg1);
    argstr = one_argument(argstr, arg2);

    //Start a new object edit session
    OBJ_DATA* obj = NULL;
    if (arg1.empty())
    {
        send_to_char("Usage: oedit <object>\n", ch);
        return;
    }

    if ( !obj && get_trust(ch) <= LEVEL_IMMORTAL )
    {
        if ( ( obj = get_obj_here( ch, arg1 ) ) == NULL )
        {
            send_to_char( "You can't find that here.\n", ch );
            return;
        }
    }


    obj = get_obj_world(ch, arg1);

    if (!obj)
    {
        send_to_char("Object not found.\n", ch);
        return;
    }

    separate_obj( obj );

    if (ch->desc->olc)
    {
        send_to_char("You are already editing something.\n", ch);
        return;
    }

    olc_start(ch, obj, get_object_schema(), &object_olc_ops);
    if (!ch->desc->olc)
        return;

   //send_to_char("Object ready for editing using olcset.\n", ch);
    olc_show(ch, "", "");
    {
        ch->desc->olc->mode = OlcEditMode::OBJECT_INLINE;
        send_to_char("Object inline editing mode enabled.\n", ch);
        send_to_char("To exit, type stop save/abort, or just save/abort.\n", ch);    

    }
}

void do_medit(CHAR_DATA* ch, char* argument)
{
    std::string arg1;
    std::string arg2;
    std::string argstr = argument;

    if (!ch || !ch->desc)
        return;

    /*
     * Existing mobile edit session commands
     */
    if (olc_mobile_in_edit_mode(ch))
    {
        if (argstr.empty())
        {
            olc_mobile_edit_help(ch);
            return;
        }

        do_olc(ch, argument);
        return;
    }

    argstr = one_argument(argstr, arg1);
    argstr = one_argument(argstr, arg2);

    /*
     * Start a new mobile edit session
     */
    CHAR_DATA* victim = nullptr;

    if (arg1.empty())
    {
        send_to_char("Usage: medit <mobile>\n", ch);
        return;
    }

    if (get_trust(ch) <= LEVEL_IMMORTAL)
    {
        if ((victim = get_char_room(ch, arg1)) == nullptr)
        {
            send_to_char("They aren't here.\n", ch);
            return;
        }
    }
    else
    {
        if ((victim = get_char_world(ch, arg1)) == nullptr)
        {
            send_to_char("No one like that in all the realms.\n", ch);
            return;
        }
    }

    if (!victim)
    {
        send_to_char("Mobile not found.\n", ch);
        return;
    }

    if (!IS_NPC(victim))
    {
        send_to_char("Medit only edits mobiles.\n", ch);
        return;
    }

    if (char_died(victim))
    {
        send_to_char("That mobile is no longer valid.\n", ch);
        return;
    }

    if (ch->desc->olc)
    {
        send_to_char("You are already editing something.\n", ch);
        return;
    }

    olc_start(ch, victim, get_mobile_schema(), &mobile_olc_ops);
    if (!ch->desc->olc)
        return;

    ch->desc->olc->mode = OlcEditMode::MOBILE_INLINE;
    send_to_char("Mobile inline editing mode enabled.\n", ch);
    send_to_char("To exit, type stop save/abort, or just save/abort.\n", ch);

    olc_show(ch, "", "");
}

void do_shopset(CHAR_DATA* ch, char* argument)
{
    std::string arg1;
    std::string arg2;
    std::string argstr = argument;
    MOB_INDEX_DATA *mob = nullptr;


    if (!ch || !ch->desc)
        return;

    /*
     * Existing shop edit session commands
     */
    if (olc_shop_in_edit_mode(ch))
    {
        if (argstr.empty())
        {
            olc_shop_edit_help(ch);
            return;
        }

        do_olc(ch, argument);
        return;
    }

    argstr = one_argument(argstr, arg1);
    argstr = one_argument(argstr, arg2);

    /*
     * Start a new shop edit session
     */
    SHOP_DATA *shop = nullptr;

    if (arg1.empty())
    {
        send_to_char("Usage: shopset <vnum of shop>\n", ch);
        return;
    }
    long vnum;
    vnum = atoi( arg1.c_str() );

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

    if (ch->desc->olc)
    {
        send_to_char("You are already editing something.\n", ch);
        return;
    }

    olc_start(ch, shop, get_shop_schema(), &shop_olc_ops);
    if (!ch->desc->olc)
        return;

    ch->desc->olc->mode = OlcEditMode::SHOP_INLINE;
    send_to_char("Shop inline editing mode enabled.\n", ch);
    send_to_char("To exit, type stop save/abort, or just save/abort.\n", ch);

    olc_show(ch, "", "");
}

void do_cfcreate(CHAR_DATA* ch, char* argument)
{
    std::string new_name;
    std::string keyword;
    std::string source_name;
    std::string argstr = argument;

    if (!ch || !ch->desc)
        return;

    if (ch->desc->olc)
    {
        send_to_char("You are already editing something.\n", ch);
        return;
    }

    argstr = one_argument(argstr, new_name);

    if (new_name.empty())
    {
        send_to_char("Usage: cfcreate <new_recipe>\n", ch);
        send_to_char("   or: cfcreate <new_recipe> copy <existing_recipe>\n", ch);
        return;
    }

    if (craft_find_recipe_for_olc(new_name))
    {
        send_to_char("A craft recipe with that name already exists.\n", ch);
        return;
    }

    argstr = one_argument(argstr, keyword);

    CraftRecipe recipe;

    if (!keyword.empty())
    {
        if (str_cmp(keyword, "copy"))
        {
            send_to_char("Syntax: cfcreate <new_recipe> copy <existing_recipe>\n", ch);
            return;
        }

        argstr = one_argument(argstr, source_name);
        if (source_name.empty())
        {
            send_to_char("Syntax: cfcreate <new_recipe> copy <existing_recipe>\n", ch);
            return;
        }

        CraftRecipe *src = craft_find_recipe_for_olc(source_name);
        if (!src)
        {
            send_to_char("Source craft recipe not found.\n", ch);
            return;
        }

        recipe = *src;
        recipe.name = new_name;
        recipe.display_name = new_name;
        recipe.disabled = false;
    }
    else
    {
        recipe.name = new_name;
        recipe.display_name = new_name;

        recipe.gsn = -1;
        recipe.disabled = false;
        recipe.room_req = CraftRoomRequirement::None;
        recipe.timer_ticks = 0;

        recipe.output_mode = CraftOutputMode::CreateFromPrototype;
        recipe.result_vnum = 0;
        recipe.result_item_type = -1;

        recipe.allow_custom_name = false;
        recipe.requires_extra_arg = false;
        recipe.extra_arg_name.clear();

        recipe.set_wear_flags.clear();
        recipe.set_objflags.clear();

        recipe.name_template.clear();
        recipe.short_template.clear();
        recipe.description_template.clear();

        recipe.slots.clear();
        recipe.derived_vars.clear();
        recipe.assignments.clear();
        recipe.affects.clear();
    }

    auto &list = craft_get_recipes_for_olc();
    list.push_back(std::move(recipe));
    CraftRecipe *created = &list.back();

    send_to_char("Craft recipe created.\n", ch);

    if (ch->desc->olc)
    {
        send_to_char("You are already editing something.\n", ch);
        return;
    }

    olc_start(ch, created, get_craft_schema(), &craft_olc_ops);
    if (!ch->desc->olc)
        return;

    ch->desc->olc->mode = OlcEditMode::CRAFT_INLINE;
    send_to_char("Entering craft editor.\n", ch);
    olc_show(ch, "", "");
}

