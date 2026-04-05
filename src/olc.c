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
****************************************************************************/

    // --------------INDEX----------------
    // OLC defines, enums, and structs
    // OLCSCHEMA Olc Schema declarations
    // GENERIC - generic format, string, flag handling that is not OLC specific
    // GENERICOLCSTRING generic format functions but dealing with OLC variables
    // OLC_FORMAT olc_format olc specific format handling
    // ROOM_OLC room_olc OlcOps functions
    // OLC_ROOM olc_room specific functions
    // OBJECT_OLC object_olc OlcOps functions
    // OLC_OBJECT olc_object specific functions
    // MOBILE_OLC mobile_olc OlcOps functions
    // OLC_MOBILE olc_mobile specific functions
    // OLC_GENERIC olc_generic functions
    // OLC_SHOW olc_show functions
    // Session Control OLC_START OLC_STOP OLC_SET
    // DO_COMMANDS DO_OLCSET DO_?EDIT DO_OLC


#include <typeinfo>
#include <cstring>
#include "mud.h"

#ifndef MIL
#define MIL MAX_INPUT_LENGTH
#endif
#ifndef MSL
#define MSL MAX_STRING_LENGTH
#endif

// External functions
extern size_t visible_length(const char *txt);
extern char *wrap_text_ex(const char *txt, int width, int flags, int indent);
extern int get_exflag( char *flag );
extern int get_risflag( char *flag );
extern int get_npc_race( char *type );
extern size_t get_langflag(char *flag);
extern bool command_is_authorized_for_char(CHAR_DATA* ch, CMDTYPE* cmd);

// Internal functions
static bool olc_extradescs_equal(const EXTRA_DESCR_DATA* a, const EXTRA_DESCR_DATA* b);
static bool olc_affects_equal(const AFFECT_DATA* a, const AFFECT_DATA* b);
static void olc_apply_flag_delta(BitSet& dst, const BitSet& edited, const BitSet& baseline);

CHAR_DATA* olc_mobile_clone(const CHAR_DATA* src);
void olc_mobile_free(CHAR_DATA* mob);
void olc_mobile_apply_instance_changes(CHAR_DATA* dst, const CHAR_DATA* src);
static bool olc_mobile_set_long_field(void* obj, const std::string& value);
static bool olc_mobile_set_race(void* obj, const std::string& value);
static bool olc_mobile_set_level(void* obj, const std::string& value);
static bool olc_mobile_set_speaking(void* obj, const std::string& value);
static bool olc_mobile_set_ris_field(void* obj, int CHAR_DATA::*member, const std::string& value);
static std::string olc_mobile_get_ris_field(void* obj, int CHAR_DATA::*member);
static bool olc_mobile_set_legacy_bits_field(void* obj, int CHAR_DATA::*member, const std::string& value, const char* const* table);
static std::string olc_mobile_get_legacy_bits_field(void* obj, int CHAR_DATA::*member, char* const* table);
static bool olc_mobile_set_spec(void* obj, const std::string& value);
static bool olc_mobile_set_spec2(void* obj, const std::string& value);
static std::string olc_mobile_get_pending_proto_die_field(CHAR_DATA* ch, CHAR_DATA* mob, const std::string& field);
static bool olc_mobile_set_pending_proto_die_field(CHAR_DATA* ch, const std::string& field, const std::string& value);
static bool olc_mobile_in_edit_mode(CHAR_DATA* ch);
bool olc_mobile_edit_affect_field(CHAR_DATA* ch, CHAR_DATA* mob);
std::string olc_mobile_affect_list_summary(CHAR_DATA* mob);
bool olc_mobile_edit_interpret(CHAR_DATA* ch, char* command, char* argument);

bool olc_object_edit_extradesc_field(CHAR_DATA* ch, OBJ_DATA* obj);
std::string olc_object_extradesc_list_summary(OBJ_DATA* obj);
bool olc_edit_object_affect_field(CHAR_DATA* ch, OBJ_DATA* obj);
std::string olc_object_affect_list_summary(OBJ_DATA* obj);
OBJ_DATA* olc_object_clone(const OBJ_DATA* src);
void olc_object_free_clone(OBJ_DATA* obj);
void olc_object_apply_instance_changes(OBJ_DATA* dst, OBJ_DATA* src);
void olc_object_apply_prototype_changes(OBJ_INDEX_DATA* dst, const OBJ_DATA* edited, const OBJ_DATA* baseline);
static bool olc_object_in_edit_mode(CHAR_DATA* ch);
bool olc_object_edit_interpret(CHAR_DATA* ch, char* command, char* argument);

bool olc_room_delete_exit(ROOM_INDEX_DATA* room, EXIT_DATA* ex);
ROOM_INDEX_DATA* olc_room_clone(const ROOM_INDEX_DATA* src);
void olc_room_apply_changes(ROOM_INDEX_DATA* src, ROOM_INDEX_DATA* dst);
static void olc_room_relink_exits(ROOM_INDEX_DATA* room);
static void olc_room_apply_pending_exit_side_effect(const OlcPendingExitSideEffect& p);
void olc_room_free(ROOM_INDEX_DATA* room);
static void olc_show_mobile_affect_help(CHAR_DATA* ch);
static void olc_show_extradesc_help(CHAR_DATA* ch);
static void olc_show_room_exit_help(CHAR_DATA* ch);
static void olc_show_object_extradesc_help(CHAR_DATA* ch);
static void olc_show_object_affect_help(CHAR_DATA* ch);
void olc_show_room_preview(CHAR_DATA* ch, ROOM_INDEX_DATA* room, const char* argument);

bool olc_commit_current(CHAR_DATA* ch);
void olc_discard_current_working_copy(CHAR_DATA* ch);
AFFECT_DATA* olc_clone_affects(AFFECT_DATA* src);
void olc_free_affects(AFFECT_DATA* af);

// --------------------------------------------
// OLC defines, enums, and structs
// --------------------------------------------

#define OLC_COL_HEADER   "&W"
#define OLC_COL_LABEL    "&c"
#define OLC_COL_STRING   "&w"
#define OLC_COL_INT      "&w"
#define OLC_COL_ENUM     "&g"
#define OLC_COL_LIST     "&g"
#define OLC_COL_HELP     "&C"
#define OLC_COL_RESET    "&x"
#define OLC_COL_PROTO_DIFF   "&C"
#define OLC_COL_PROTO_SAME   "&w"

enum class MatchType
{
    NONE,
    PREFIX,
    EXACT
};


struct OlcItemValueInfo
{
    int item_type;
    bool used;
    bool values_used;
    const char* labels[6];
};

struct MatchResult
{
    MatchType type = MatchType::NONE;
    size_t match_length = 0; // for prefix ranking
};

struct OlcExitCreateOptions
{
    bool two_way = false;

    bool has_flags = false;
    int flags = 0;

    bool has_key = false;
    int key = -1;

    bool has_keyword = false;
    std::string keyword;
};

struct OlcExitSelector
{
    EXIT_DATA* ex = nullptr;
    int dir = -1;
    bool has_dir = false;
};

// --------------------------------------------
// OLCSCHEMA Olc Schema declarations
// --------------------------------------------

static OlcField make_olc_object_value_field(
    const char* name,
    int index,
    const char* help)
{
    return OlcField{
        name,
        OlcValueType::INT,
        nullptr,
        OlcMetaType::NONE,
        nullptr,
        [index](void* obj, const std::string& value) -> bool
        {
            auto o = static_cast<OBJ_DATA*>(obj);
            return set_int_field(o->value[index], value, INT_MIN, INT_MAX);
        },
        [index](void* obj) -> std::string
        {
            auto o = static_cast<OBJ_DATA*>(obj);
            return get_int_field(o->value[index]);
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

const OlcSchema* get_room_schema()
{
    static std::vector<OlcField> room_fields =
    {
        make_olc_string_field<ROOM_INDEX_DATA>("name", &ROOM_INDEX_DATA::name, "Room Name" ),
        make_olc_editor_field<ROOM_INDEX_DATA>("description", &ROOM_INDEX_DATA::description, SUB_ROOM_DESC, 
            "Room's long description which shows when looking/moving" ),
        make_olc_custom_editor_field<ROOM_INDEX_DATA>("extradesc", OlcMetaType::EXTRA_DESC_LIST, 
            [](CHAR_DATA* ch, ROOM_INDEX_DATA* room) -> bool { return olc_room_edit_extradesc_field(ch, room); },
            [](ROOM_INDEX_DATA* room) -> std::string { return olc_room_extradesc_list_summary(room); }, SUB_ROOM_EXTRA,
            "These provide extra 'look' targets that can provide more description",
            true ),
        make_olc_enum_flag_field<ROOM_INDEX_DATA>( "sector_type", &ROOM_INDEX_DATA::sector_type, sect_types, 
            "Room Terrain - Determines ambiance messages" ),
        make_olc_flag_field<ROOM_INDEX_DATA>( "flags", &ROOM_INDEX_DATA::room_flags, r_flags, 
            "Room flags: See HELP ROOMFLAGS for flag meaning" ),
        make_olc_int_field<ROOM_INDEX_DATA>( "tunnel", &ROOM_INDEX_DATA::tunnel, "Max occupants that can be in the room", 0, 200 ),
        make_olc_int_field<ROOM_INDEX_DATA>( "televnum", &ROOM_INDEX_DATA::tele_vnum, "Teleportation Destination", INT_MIN, INT_MAX ),
        make_olc_int_field<ROOM_INDEX_DATA>( "teledelay", &ROOM_INDEX_DATA::tele_delay, "Delay before teleportation", INT_MIN, INT_MAX ),
        make_olc_custom_editor_field<ROOM_INDEX_DATA>( "exit", OlcMetaType::EXIT_LIST, 
            [](CHAR_DATA* ch, ROOM_INDEX_DATA* room) -> bool { return olc_room_edit_exit_field(ch, room); },
            [](ROOM_INDEX_DATA* room) -> std::string { return olc_room_exit_list_summary(room); },
            0, "Manage room exits", true ),
        make_olc_custom_editor_field<ROOM_INDEX_DATA>( "bexit", OlcMetaType::EXIT_LIST,
            [](CHAR_DATA* ch, ROOM_INDEX_DATA* room) -> bool { return olc_room_edit_bexit_field(ch, room); },
            [](ROOM_INDEX_DATA* room) -> std::string { return olc_room_exit_list_summary(room); },
            0, "Manage reverse/bidirectional exit changes", true ),
    };

    static OlcSchema room_schema =
    {
        "room",
        &room_fields
    };

    return &room_schema;
}

const OlcSchema* get_object_schema()
{
    static std::vector<OlcField> object_fields =
    {
        make_olc_string_field<OBJ_DATA>( "name", &OBJ_DATA::name, "Object keywords" ),
        make_olc_string_field<OBJ_DATA>( "short", &OBJ_DATA::short_descr, "Short description" ),
        make_olc_editor_field<OBJ_DATA>( "description", &OBJ_DATA::description, SUB_OBJ_LONG, "Long object description" ),
        make_olc_string_field<OBJ_DATA>( "action", &OBJ_DATA::action_desc, "Action description" ),
        make_olc_enum_flag_field<OBJ_DATA>( "type", &OBJ_DATA::item_type, o_types, "Object type" ),
        make_olc_flag_field<OBJ_DATA>( "wearflags", &OBJ_DATA::wear_flags, w_flags, "Wear flags" ),
        make_olc_flag_field<OBJ_DATA>( "flags", &OBJ_DATA::objflags, obj_flag_table, "Object flags" ),
        make_olc_int_field<OBJ_DATA>( "wearloc", &OBJ_DATA::wear_loc, "Current wear location" ),
        make_olc_int_field<OBJ_DATA>( "weight", &OBJ_DATA::weight, "Weight" ),
        make_olc_int_field<OBJ_DATA>( "cost", &OBJ_DATA::cost, "Cost" ),
        make_olc_int_field<OBJ_DATA>( "level", &OBJ_DATA::level, "Level" ),
        make_olc_int_field<OBJ_DATA>( "timer", &OBJ_DATA::timer, "Timer" ),
        make_olc_int_field<OBJ_DATA>( "count", &OBJ_DATA::count, "Count" ),
        make_olc_object_value_field("value0", 0, "Object value[0]"),
        make_olc_object_value_field("value1", 1, "Object value[1]"),
        make_olc_object_value_field("value2", 2, "Object value[2]"),
        make_olc_object_value_field("value3", 3, "Object value[3]"),
        make_olc_object_value_field("value4", 4, "Object value[4]"),
        make_olc_object_value_field("value5", 5, "Object value[5]"),
        make_olc_custom_editor_field<OBJ_DATA>( "extradesc", OlcMetaType::EXTRA_DESC_LIST,
            [](CHAR_DATA* ch, OBJ_DATA* obj) -> bool { return olc_object_edit_extradesc_field(ch, obj); },
            [](OBJ_DATA* obj) -> std::string { return olc_object_extradesc_list_summary(obj); },
            SUB_OBJ_EXTRA, "Object extra descriptions", true ),
        make_olc_custom_editor_field<OBJ_DATA>( "affect", OlcMetaType::OBJ_AFFECT_LIST,
            [](CHAR_DATA* ch, OBJ_DATA* obj) -> bool { return olc_edit_object_affect_field(ch, obj); },
            [](OBJ_DATA* obj) -> std::string {
                return olc_object_affect_list_summary(obj); },
            0, "Object affects", true )        
    };

    static OlcSchema object_schema =
    {
        "object",
        &object_fields
    };

    return &object_schema;
}


const OlcSchema* get_mobile_schema()
{
    static std::vector<OlcField> mob_fields =
    {
        make_olc_string_field<CHAR_DATA>( "name", &CHAR_DATA::name, "Mobile keywords" ),
        make_olc_string_field<CHAR_DATA>( "short", &CHAR_DATA::short_descr, "Short description" ),
        OlcField{ "long", OlcValueType::STRING, nullptr, OlcMetaType::NONE, nullptr,
            [](void* obj, const std::string& value) -> bool { return olc_mobile_set_long_field(obj, value); },
            [](void* obj) -> std::string { auto mob = static_cast<CHAR_DATA*>(obj); return mob->long_descr ? mob->long_descr : ""; },
            nullptr, nullptr, 0, "Mob long description", INT_MIN, INT_MAX, false },
        make_olc_editor_field<CHAR_DATA>( "description", &CHAR_DATA::description, SUB_MOB_DESC, "Long description" ),
        make_olc_int_field<CHAR_DATA>( "sex", &CHAR_DATA::sex, "Sex", 0, 2 ),
        OlcField{ "race", OlcValueType::ENUM, nullptr, OlcMetaType::NONE, nullptr,
            [](void* obj, const std::string& value) -> bool { return olc_mobile_set_race(obj, value); },
            [](void* obj) -> std::string {
                auto mob = static_cast<CHAR_DATA*>(obj);
                int r = mob->race;
                if (r >= 0 && r < MAX_NPC_RACE)
                    return get_flag_name(npc_race, r, MAX_NPC_RACE);
                return std::to_string(r);
            },
            nullptr, nullptr, 0, "NPC race", INT_MIN, INT_MAX, false },
        OlcField{ "level", OlcValueType::INT, nullptr, OlcMetaType::NONE, nullptr,
            [](void* obj, const std::string& value) -> bool { return olc_mobile_set_level(obj, value); },
            [](void* obj) -> std::string { auto mob = static_cast<CHAR_DATA*>(obj); return get_int_field(mob->top_level); },
            nullptr, nullptr, 0, "Top level", 0, LEVEL_AVATAR + 5, false },
        make_olc_int_field<CHAR_DATA>( "alignment", &CHAR_DATA::alignment, "Alignment", -1000, 1000 ),
        make_olc_int_field<CHAR_DATA>( "armor", &CHAR_DATA::armor, "Armor class", -300, 300 ),
        make_olc_int_field<CHAR_DATA>( "hitroll", &CHAR_DATA::hitroll, "Hitroll", 0, 85 ),
        make_olc_int_field<CHAR_DATA>( "damroll", &CHAR_DATA::damroll, "Damroll", 0, 65 ),
        make_olc_int_field<CHAR_DATA>( "hitplus", &CHAR_DATA::hitplus, "Hit point bonus" ),
        make_olc_int_field<CHAR_DATA>( "damplus", &CHAR_DATA::damplus, "Damage bonus" ),
        make_olc_int_field<CHAR_DATA>( "numattacks", &CHAR_DATA::numattacks, "Number of attacks", 0, 20 ),
        make_olc_int_field<CHAR_DATA>( "position", &CHAR_DATA::position, "Current/default position", 0, POS_STANDING ),
        make_olc_int_field<CHAR_DATA>( "defposition", &CHAR_DATA::defposition, "Default standing position", 0, POS_STANDING ),
        make_olc_int_field<CHAR_DATA>( "height", &CHAR_DATA::height, "Height" ),
        make_olc_int_field<CHAR_DATA>( "weight", &CHAR_DATA::weight, "Weight" ),
        make_olc_int_field<CHAR_DATA>( "credits", &CHAR_DATA::gold, "Credits/Money" ),
        make_olc_int_field<CHAR_DATA>( "str", &CHAR_DATA::perm_str, "Strength", 1, 25 ),
        make_olc_int_field<CHAR_DATA>( "int", &CHAR_DATA::perm_int, "Intelligence", 1, 25 ),
        make_olc_int_field<CHAR_DATA>( "wis", &CHAR_DATA::perm_wis, "Wisdom", 1, 25 ),
        make_olc_int_field<CHAR_DATA>( "dex", &CHAR_DATA::perm_dex, "Dexterity", 1, 25 ),
        make_olc_int_field<CHAR_DATA>( "con", &CHAR_DATA::perm_con, "Constitution", 1, 25 ),
        make_olc_int_field<CHAR_DATA>( "cha", &CHAR_DATA::perm_cha, "Charisma", 1, 25 ),
        make_olc_int_field<CHAR_DATA>( "lck", &CHAR_DATA::perm_lck, "Luck", 1, 25 ),
        make_olc_int_field<CHAR_DATA>( "frc", &CHAR_DATA::perm_frc, "Force affinity", 0, 20 ),
        make_olc_int_field<CHAR_DATA>( "sav1", &CHAR_DATA::saving_poison_death, "Save vs poison", -30, 30 ),
        make_olc_int_field<CHAR_DATA>( "sav2", &CHAR_DATA::saving_wand, "Save vs wand", -30, 30 ),
        make_olc_int_field<CHAR_DATA>( "sav3", &CHAR_DATA::saving_para_petri, "Save vs paralysis/petrification", -30, 30 ),
        make_olc_int_field<CHAR_DATA>( "sav4", &CHAR_DATA::saving_breath, "Save vs breath", -30, 30 ),
        make_olc_int_field<CHAR_DATA>( "sav5", &CHAR_DATA::saving_spell_staff, "Save vs spell/staff", -30, 30 ),
        make_olc_flag_field<CHAR_DATA>( "flags", &CHAR_DATA::act, act_flags, "NPC act flags" ),
        make_olc_flag_field<CHAR_DATA>( "affected", &CHAR_DATA::affected_by, aff_flags, "Affect flags" ),
        make_olc_flag_field<CHAR_DATA>( "vip", &CHAR_DATA::vip_flags, planet_flags, "VIP flags" ),
        make_olc_int_field<CHAR_DATA>( "force", &CHAR_DATA::max_mana, "Force points" ),
        make_olc_int_field<CHAR_DATA>( "move", &CHAR_DATA::max_move, "Movement points" ),
        make_olc_int_field<CHAR_DATA>( "hp", &CHAR_DATA::max_hit, "Max HP" ),
        make_olc_int_field<CHAR_DATA>( "carry_weight", &CHAR_DATA::carry_weight, "Carry limits" ),
        make_olc_flag_field<CHAR_DATA>( "speaks", &CHAR_DATA::speaks, lang_names, "Languages spoken" ),
        OlcField{ "resistant", OlcValueType::FLAG, (const void*)ris_flags, OlcMetaType::ENUM_LEGACY, nullptr, nullptr,
            [](void* obj) -> std::string { return olc_mobile_get_ris_field(obj, &CHAR_DATA::resistant); },
            [](CHAR_DATA* /*ch*/, void* obj, const std::string& value) -> bool { return olc_mobile_set_ris_field(obj, &CHAR_DATA::resistant, value); },
            nullptr, 0, "Resistances (RIS flags)", INT_MIN, INT_MAX, false },
        OlcField{ "immune", OlcValueType::FLAG, (const void*)ris_flags, OlcMetaType::ENUM_LEGACY, nullptr, nullptr,
            [](void* obj) -> std::string { return olc_mobile_get_ris_field(obj, &CHAR_DATA::immune); },
            [](CHAR_DATA* /*ch*/, void* obj, const std::string& value) -> bool { return olc_mobile_set_ris_field(obj, &CHAR_DATA::immune, value); },
            nullptr, 0, "Immunities (RIS flags)", INT_MIN, INT_MAX, false },
        OlcField{ "susceptible", OlcValueType::FLAG, (const void*)ris_flags, OlcMetaType::ENUM_LEGACY, nullptr, nullptr,
            [](void* obj) -> std::string { return olc_mobile_get_ris_field(obj, &CHAR_DATA::susceptible); },
            [](CHAR_DATA* /*ch*/, void* obj, const std::string& value) -> bool { return olc_mobile_set_ris_field(obj, &CHAR_DATA::susceptible, value); },
            nullptr, 0, "Susceptibilities (RIS flags)", INT_MIN, INT_MAX, false },
        OlcField{ "speaking", OlcValueType::ENUM, (const void*)lang_names, OlcMetaType::FLAG_TABLE, nullptr,
            [](void* obj, const std::string& value) -> bool { return olc_mobile_set_speaking(obj, value); },
            [](void* obj) -> std::string { auto mob = static_cast<CHAR_DATA*>(obj); return lang_names[mob->speaking].name; },
            nullptr, nullptr, 0, "Current spoken language", INT_MIN, INT_MAX, false },
        OlcField{ "part", OlcValueType::FLAG, (const void*)part_flags, OlcMetaType::ENUM_LEGACY, nullptr, nullptr,
            [](void* obj) -> std::string { return olc_mobile_get_legacy_bits_field(obj, &CHAR_DATA::xflags, part_flags); },
            [](CHAR_DATA* /*ch*/, void* obj, const std::string& value) -> bool { return olc_mobile_set_legacy_bits_field(obj, &CHAR_DATA::xflags, value, part_flags); },
            nullptr, 0, "Body parts", INT_MIN, INT_MAX, false },
        OlcField{ "attack", OlcValueType::FLAG, (const void*)attack_flags, OlcMetaType::ENUM_LEGACY, nullptr, nullptr,
            [](void* obj) -> std::string { return olc_mobile_get_legacy_bits_field(obj, &CHAR_DATA::attacks, attack_flags); },
            [](CHAR_DATA* /*ch*/, void* obj, const std::string& value) -> bool { return olc_mobile_set_legacy_bits_field(obj, &CHAR_DATA::attacks, value, attack_flags); },
            nullptr, 0, "Attack flags", INT_MIN, INT_MAX, false },
        OlcField{ "defense", OlcValueType::FLAG, (const void*)defense_flags, OlcMetaType::ENUM_LEGACY, nullptr, nullptr,
            [](void* obj) -> std::string { return olc_mobile_get_legacy_bits_field(obj, &CHAR_DATA::defenses, defense_flags); },
            [](CHAR_DATA* /*ch*/, void* obj, const std::string& value) -> bool { return olc_mobile_set_legacy_bits_field(obj, &CHAR_DATA::defenses, value, defense_flags); },
            nullptr, 0, "Defense flags", INT_MIN, INT_MAX, false },      
        OlcField{ "spec", OlcValueType::STRING, nullptr, OlcMetaType::NONE, nullptr, nullptr,
            [](void* obj) -> std::string { auto mob = static_cast<CHAR_DATA*>(obj); return mob->spec_fun ? lookup_spec( mob->spec_fun ) : "none"; },
            [](CHAR_DATA* /*ch*/, void* obj, const std::string& value) -> bool { return olc_mobile_set_spec(obj, value); },
            nullptr, 0, "Primary special function name, or 'none'", INT_MIN, INT_MAX, false },
        OlcField{ "spec2", OlcValueType::STRING, nullptr, OlcMetaType::NONE, nullptr, nullptr,
            [](void* obj) -> std::string { auto mob = static_cast<CHAR_DATA*>(obj); return mob->spec_2 ? lookup_spec( mob->spec_2 ) : "none"; },
            [](CHAR_DATA* /*ch*/, void* obj, const std::string& value) -> bool { return olc_mobile_set_spec2(obj, value); },
            nullptr, 0, "Secondary special function name, or 'none'", INT_MIN, INT_MAX, false },            
        OlcField{ "hitnodice", OlcValueType::INT, nullptr, OlcMetaType::NONE, nullptr, nullptr, nullptr,
            [](CHAR_DATA* ch, void* /*obj*/, const std::string& value) -> bool { return olc_mobile_set_pending_proto_die_field(ch, "hitnodice", value); },
            [](CHAR_DATA* ch, void* obj) -> std::string { return olc_mobile_get_pending_proto_die_field( ch, static_cast<CHAR_DATA*>(obj), "hitnodice"); },
            0, "Prototype hit dice count", 0, 32767, false },
        OlcField{ "hitsizedice", OlcValueType::INT, nullptr, OlcMetaType::NONE, nullptr, nullptr, nullptr,
            [](CHAR_DATA* ch, void* /*obj*/, const std::string& value) -> bool { return olc_mobile_set_pending_proto_die_field(ch, "hitsizedice", value); },
            [](CHAR_DATA* ch, void* obj) -> std::string { return olc_mobile_get_pending_proto_die_field( ch, static_cast<CHAR_DATA*>(obj), "hitsizedice"); },
            0, "Prototype hit dice size", 0, 32767, false },
        OlcField{ "damnodice", OlcValueType::INT, nullptr, OlcMetaType::NONE, nullptr, nullptr, nullptr,
            [](CHAR_DATA* ch, void* /*obj*/, const std::string& value) -> bool { return olc_mobile_set_pending_proto_die_field(ch, "damnodice", value); },
            [](CHAR_DATA* ch, void* obj) -> std::string { return olc_mobile_get_pending_proto_die_field( ch, static_cast<CHAR_DATA*>(obj), "damnodice"); },
            0, "Prototype damage dice count", 0, 32767, false  },
        OlcField{ "damsizedice", OlcValueType::INT, nullptr, OlcMetaType::NONE, nullptr, nullptr, nullptr,
            [](CHAR_DATA* ch, void* /*obj*/, const std::string& value) -> bool { return olc_mobile_set_pending_proto_die_field(ch, "damsizedice", value); },
            [](CHAR_DATA* ch, void* obj) -> std::string { return olc_mobile_get_pending_proto_die_field( ch, static_cast<CHAR_DATA*>(obj), "damsizedice"); },
            0, "Prototype damage dice size", 0, 32767, false },   
        make_olc_custom_editor_field<CHAR_DATA>( "affect", OlcMetaType::MOB_AFFECT_LIST,
            [](CHAR_DATA* ch, CHAR_DATA* mob) -> bool { return olc_mobile_edit_affect_field(ch, mob); },
            [](CHAR_DATA* mob) -> std::string { return olc_mobile_affect_list_summary(mob); },
            0, "Mobile affects", true ),            
    };

    static OlcSchema mob_schema =
    {
        "mobile",
        &mob_fields
    };

    return &mob_schema;
}
// Value0 through 5 use description mapping, used for help files..
const OlcItemValueInfo g_olc_item_value_info[] =
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

// --------------------------------------------
// GENERIC - generic format, string, flag handling that is not OLC specific
// --------------------------------------------
static bool same_str(const char* a, const char* b)
{
    if (a == b)
        return true;

    if (!a)
        a = "";
    if (!b)
        b = "";

    return !str_cmp(a, b);
}

static void olc_replace_string(char*& dst, const char* src)
{
    if (dst)
        STRFREE(dst);

    dst = (src && src[0] != '\0') ? STRALLOC((char*)src) : nullptr;
}

static void olc_replace_string_idx(char*& dst, const char* src)
{
    if (dst)
        STRFREE(dst);

    dst = (src && src[0] != '\0') ? STRALLOC((char*)src) : nullptr;
}

bool olc_command_matches(const char* input, const char* word)
{
    return input && word && !str_prefix(input, word);
}

bool enum_from_string_legacy(const std::string& input, int& out, const char* const* table)
{
    for (int i = 0; table[i] != nullptr; ++i)
    {
        if (!str_cmp_utf8(input.c_str(), table[i]))
        {
            out = i;
            return true;
        }
    }
    return false;
}

bool enum_from_string_flag(const std::string& input, int& out, const flag_name *table)
{
    const flag_name *f;
    f = find_flag(table,input);
    if (!f)
        return false;
    out = f->bit;
    return true;
}

std::string enum_to_string_legacy(int value, const char* const* table)
{
    if (!table || value < 0)
        return std::to_string(value);

    for (int i = 0; table[i] != nullptr; ++i)
    {
        if (i == value)
            return table[i];
    }

    return std::to_string(value);
}

std::string enum_to_string_flag(int value, const flag_name *table)
{
    if (!table || value < 0)
        return std::to_string(value);

    for (int i = 0; table[i].name != nullptr; ++i)
    {
        if (i == value)
            return table[i].name;
    }

    return std::to_string(value);
}

bool bitset_apply_from_string(BitSet& bs, const std::string& input, const flag_name* table)
{
    std::istringstream iss(input);
    std::string token;
    std::vector<std::string> unknown;
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

        const flag_name* flag = nullptr;

        for (size_t i = 0; table[i].name != nullptr; ++i)
        {
            if (!str_cmp_utf8(token.c_str(), table[i].name))
            {
                flag = &table[i];
                break;
            }
        }

        if (!flag)
        {
            unknown.push_back(token);
            continue;
        }

        switch (op)
        {
            case '-':
                bs.clear(flag->bit);
                break;

            case '!':
                bs.toggle(flag->bit);
                break;

            case '+':
            default:
                bs.set(flag->bit);
                break;
        }

        changed = true;
    }

    if (!unknown.empty()) // Not currently handling this as there's nobody to send it to, but may change this later - DV
    {
        return changed;
    }

    return changed;
}

std::vector<std::string> flag_values_vec(const char* const* table)
{
    std::vector<std::string> result;

    if (!table)
        return result;

    for (int i = 0; table[i] != nullptr; ++i)
    {
        result.emplace_back(table[i]);
    }

    return result;
}

std::vector<std::string> split_words(const std::string& str)
{
    std::vector<std::string> words;
    std::istringstream iss(str);
    std::string word;

    while (iss >> word)
        words.push_back(word);

    return words;
}

MatchResult match_keywords(const std::string& keyword_list, const std::string& input)
{
    MatchResult result;

    if (keyword_list.empty() || input.empty())
        return result;

    auto words = split_words(keyword_list);

    for (const auto& word : words)
    {
        // Exact match
        if (!str_cmp(word.c_str(), input.c_str()))
        {
            return { MatchType::EXACT, word.length() };
        }

        // Prefix match
        if (!str_prefix(input.c_str(), word.c_str()))
        {
            if (word.length() > result.match_length)
            {
                result.type = MatchType::PREFIX;
                result.match_length = word.length();
            }
        }
    }

    return result;
}


int dir_lookup(const std::string& dir)
{
    if (dir.empty())
        return -1;

    /* Normalize to lowercase */
    std::string d = dir;
    std::transform(d.begin(), d.end(), d.begin(), ::tolower);

    /* Handle ? / somewhere */
    if (d == "?" || d == "somewhere")
        return 10; // adjust if your somewhere index differs

    /* Direction aliases */
    struct DirAlias
    {
        const char* name;
        int dir;
    };

    static const DirAlias aliases[] =
    {
        { "n",  DIR_NORTH }, { "north", DIR_NORTH },
        { "e",  DIR_EAST  }, { "east",  DIR_EAST  },
        { "s",  DIR_SOUTH }, { "south", DIR_SOUTH },
        { "w",  DIR_WEST  }, { "west",  DIR_WEST  },

        { "u",  DIR_UP    }, { "up",    DIR_UP    },
        { "d",  DIR_DOWN  }, { "down",  DIR_DOWN  },

        { "ne", DIR_NORTHEAST }, { "northeast", DIR_NORTHEAST },
        { "nw", DIR_NORTHWEST }, { "northwest", DIR_NORTHWEST },
        { "se", DIR_SOUTHEAST }, { "southeast", DIR_SOUTHEAST },
        { "sw", DIR_SOUTHWEST }, { "southwest", DIR_SOUTHWEST },

        { nullptr, -1 }
    };

    /* Exact match first */
    for (int i = 0; aliases[i].name != nullptr; ++i)
    {
        if (d == aliases[i].name)
            return aliases[i].dir;
    }

    /* Fallback to existing full-name matching (if any) */
    for (int i = 0; i <= MAX_DIR+1; ++i)
    {
        if (!str_cmp(d.c_str(), dir_name[i]))
            return i;
    }

    return -1;
}

int get_rev_dir(int dir)
{
    return (dir >= 0 && dir <= MAX_DIR+1) ? rev_dir[dir] : -1;
}

static const char* exit_dir_label(int dir)
{
    if (dir >= 0 && dir <= MAX_DIR+1)
    {
        if (!str_cmp(dir_name[dir], "somewhere"))
            return "?";
        return dir_name[dir];
    }
    return "?";
}

EXIT_DATA* find_exit(ROOM_INDEX_DATA* room, int dir)
{
    return get_exit(room, dir);
}

static EXIT_DATA* find_exit_by_number(ROOM_INDEX_DATA* room, int index)
{
    if (!room || index <= 0)
        return nullptr;

    int count = 0;
    for (EXIT_DATA* ex = room->first_exit; ex; ex = ex->next)
    {
        ++count;
        if (count == index)
            return ex;
    }

    return nullptr;
}

static EXIT_DATA* find_exit_by_dir_index(ROOM_INDEX_DATA* room, int dir, int index)
{
    if (!room || dir < 0 || index <= 0)
        return nullptr;

    int count = 0;
    for (EXIT_DATA* ex = room->first_exit; ex; ex = ex->next)
    {
        if (ex->vdir != dir)
            continue;

        ++count;
        if (count == index)
            return ex;
    }

    return nullptr;
}

static EXIT_DATA* find_exit_by_keyword(ROOM_INDEX_DATA* room, const std::string& keyword)
{
    if (!room || keyword.empty())
        return nullptr;

    for (EXIT_DATA* ex = room->first_exit; ex; ex = ex->next)
    {
        if (!ex->keyword || ex->keyword[0] == '\0')
            continue;

        if (is_name(keyword.c_str(), ex->keyword))
            return ex;
    }

    return nullptr;
}

static bool olc_is_direction_alias(const char* cmd)
{
    if (!cmd || cmd[0] == '\0')
        return false;

    std::string input = cmd;
    std::transform(input.begin(), input.end(), input.begin(), ::tolower);

    for (int i = 0; i <= MAX_DIR; ++i)
    {
        if (!dir_name[i])
            continue;

        std::string dir = dir_name[i];

        // normalize "somewhere" → "?"
        if (dir == "somewhere")
            dir = "?";

        // prefix match: cmd is a prefix of direction
        if (dir.compare(0, input.length(), input) == 0)
            return true;
    }

    return false;
}

static bool olc_str_eq(const char* a, const char* b)
{
    if (!a) a = "";
    if (!b) b = "";
    return !str_cmp(a, b);
}

static std::vector<std::string> olc_wrap_value_pairs(
    const std::vector<std::string>& pairs,
    int width)
{
    std::vector<std::string> lines;
    std::string current;
    int current_len = 0;

    for (const auto& pair : pairs)
    {
        int pair_len = visible_length(pair.c_str());
        int sep_len = current.empty() ? 0 : 2; /* "  " */

        if (!current.empty() && current_len + sep_len + pair_len > width)
        {
            lines.push_back(current);
            current = pair;
            current_len = pair_len;
        }
        else
        {
            if (!current.empty())
            {
                current += "  ";
                current_len += 2;
            }

            current += pair;
            current_len += pair_len;
        }
    }

    if (!current.empty())
        lines.push_back(current);

    return lines;
}



// --------------------------------------------
// GENERICOLCSTRING generic string functions - generic functions but dealing with OLC variables
// --------------------------------------------

static std::vector<std::string> olc_enum_values_vec(const OlcField& f)
{
    std::vector<std::string> out;

    switch (f.meta_type)
    {
        case OlcMetaType::ENUM_FLAG:
        {
            auto table = static_cast<const flag_name*>(f.meta);
            for (size_t i = 0; table[i].name; ++i)
                out.push_back(table[i].name);
            break;
        }
        case OlcMetaType::FLAG_TABLE:
        {
            auto table = static_cast<const flag_name*>(f.meta);
            for (size_t i = 0; table[i].name; ++i)
                out.push_back(table[i].name);
            break;
        }
        case OlcMetaType::ENUM_LEGACY:
        {
            auto table = static_cast<const char* const*>(f.meta);
            for (int i = 0; table[i]; ++i)
                out.push_back(table[i]);
            break;
        }

        default:
            break;
    }

    return out;
}

static std::vector<std::string> olc_enum_suggestions(const std::string& input, const OlcField& f)
{
    std::vector<std::string> matches;
    auto values = olc_enum_values_vec(f);

    for (const auto& v : values)
    {
        if (str_prefix_utf8(input.c_str(), v.c_str()) == false) // starts with
            matches.push_back(v);
    }

    return matches;
}

std::vector<std::string> olc_all_field_names(const OlcSchema* schema)
{
    std::vector<std::string> out;

    for (const auto& f : *schema->fields)
        out.push_back(f.name);

    return out;
}

std::vector<std::string> olc_field_suggestions(const OlcSchema* schema, const std::string& input)
{
    std::vector<std::string> matches;

    for (const auto& f : *schema->fields)
    {
        // prefix match
        if (str_prefix(input.c_str(), f.name) == false)
        {
            matches.push_back(f.name);
            continue;
        }

        // substring fallback
        if (strcasestr(f.name, input.c_str()))
            matches.push_back(f.name);
    }
    std::sort(matches.begin(), matches.end());

    return matches;
}

const OlcField* olc_find_field_fuzzy(const OlcSchema* schema, const std::string& input)
{
    const OlcField* prefix_match = nullptr;

    for (const auto& f : *schema->fields)
    {
        // Exact match (fast path)
        if (!str_cmp(input.c_str(), f.name))
            return &f;

        // Prefix match
        if (str_prefix(input.c_str(), f.name) == false)
        {
            if (prefix_match) // ambiguous
                return nullptr;

            prefix_match = &f;
        }
    }

    return prefix_match; // may be nullptr
}

static std::vector<std::string> olc_ambiguous_field_matches(const OlcSchema* schema, const std::string& input)
{
    std::vector<std::string> matches;

    if (!schema || input.empty())
        return matches;

    for (const auto& f : *schema->fields)
    {
        if (!str_prefix(input.c_str(), f.name))
            matches.push_back(f.name);
    }

    std::sort(matches.begin(), matches.end());
    return matches;
}

static bool olc_field_name_is_ambiguous(const OlcSchema* schema, const std::string& input)
{
    if (!schema || input.empty())
        return false;

    int matches = 0;

    for (const auto& f : *schema->fields)
    {
        if (!str_prefix(input.c_str(), f.name))
            ++matches;
    }

    return matches > 1;
}


// --------------------------------------------
// OLC_FORMAT olc_format olc specific format handling
// --------------------------------------------
static std::vector<std::string> olc_format_wrap_text(
    const std::string& text,
    int width,
    int indent)
{
    std::vector<std::string> lines;

    if (text.empty())
        return lines;

    int flags = WRAP_HANGING_INDENT;

    if (indent > 0)
        flags |= WRAP_HANGING_INDENT;

    char* wrapped = wrap_text_ex(text.c_str(), width, flags, indent);

    if (!wrapped)
        return lines;

    // Split into lines
    char* p = wrapped;
    char* start = p;

    while (*p)
    {
        if (*p == '\n' || *p == '\r')
        {
            if (p > start)
                lines.emplace_back(start, p - start);

            // skip CRLF combos
            while (*p == '\n' || *p == '\r')
                ++p;

            start = p;
        }
        else
        {
            ++p;
        }
    }

    // last line
    if (p > start)
        lines.emplace_back(start, p - start);

    STR_DISPOSE(wrapped); // or free(), depending on your allocator

    return lines;
}

static std::vector<std::string> olc_format_line(
    const char* name,
    const std::string& value,
    const char* val_color,
    int max_name_len,
    int term_width)
{
    std::vector<std::string> out;

    int label_width = max_name_len;
    int prefix_len = label_width + 3; // "name : "

    int available = term_width - prefix_len;
    if (available < 20)
        available = 20;

    auto wrapped = olc_format_wrap_text(value, available, prefix_len);

    char buf[MSL];

    // First line
    snprintf(buf, sizeof(buf),
        "%s%-*s%s : %s%s%s",
        OLC_COL_LABEL,
        label_width,
        name,
        OLC_COL_RESET,
        val_color,
        wrapped.empty() ? "None" : wrapped[0].c_str(),
        OLC_COL_RESET
    );

    out.push_back(buf);

    // Continuation lines
    for (size_t i = 1; i < wrapped.size(); ++i)
    {
        snprintf(buf, sizeof(buf),
            "%*s   %s%s%s",
            label_width,
            "",
            val_color,
            wrapped[i].c_str(),
            OLC_COL_RESET
        );

        out.push_back(buf);
    }

    return out;
}

std::string olc_format_columns(
    const std::vector<std::string>& items,
    int term_width,
    int indent)
{
    if (items.empty())
        return "";

    // Compute max visible width of items
    size_t max_len = 0;
    for (const auto& s : items)
        max_len = std::max(max_len, (size_t)visible_length(s.c_str()));

    max_len += 2; // spacing between columns

    // Compute how many columns fit
    int usable_width = term_width - indent;
    if (usable_width < (int)max_len)
        usable_width = max_len;

    size_t cols = (size_t)(usable_width / max_len);
    if (cols == 0)
        cols = 1;

    std::string out;
    std::string pad(indent, ' ');

    for (size_t i = 0; i < items.size(); ++i)
    {
        // Start new row
        if (i % cols == 0)
            out += pad;

        const std::string& item = items[i];
        out += item;

        int vis_len = visible_length(item.c_str());
        int spacing = max_len - vis_len;

        // Pad unless last column
        if ((i % cols) != (cols - 1))
            out.append(spacing, ' ');

        // End row
        if ((i % cols) == (cols - 1))
            out += "\n";
    }

    // Final newline if last row incomplete
    if (items.size() % cols != 0)
        out += "\n";

    return out;
}

static std::string  olc_format_exit_flags_list()
{
    std::vector<std::string> vals = flag_values_vec(ex_flags);
    if (vals.empty())
        return "";

    return olc_format_columns(vals, 80, 2);
}


static bool olc_format_parse_strict_int(const std::string& s, int& out)
{
    if (s.empty())
        return false;

    char* end = nullptr;
    long val = strtol(s.c_str(), &end, 10);

    if (!end || *end != '\0')
        return false;

    out = static_cast<int>(val);
    return true;
}

static std::string olc_format_trim_left(std::string s)
{
    size_t pos = s.find_first_not_of(' ');
    if (pos == std::string::npos)
        return "";
    s.erase(0, pos);
    return s;
}

static std::vector<std::string> olc_format_pending_exit_side_effects(
    CHAR_DATA* ch,
    size_t label_width,
    int term_width)
{
    std::vector<std::string> out;

    if (!ch || !ch->desc || !ch->desc->olc)
        return out;

    const auto& pending = ch->desc->olc->pending_exit_side_effects;
    if (pending.empty())
        return out;

    bool first_line = true;

    for (const auto& p : pending)
    {
        std::string from_dir = exit_dir_label(p.from_dir);
        std::string rev_dir = exit_dir_label(get_rev_dir(p.from_dir));
        std::string flags = flag_string(p.final_exit_info, (char * const *)ex_flags);
        if (flags.empty())
            flags = "-";

        std::string keyword = p.final_keyword.empty() ? "-" : p.final_keyword;
        std::string text;

        if (p.type == OlcExitSideEffectType::DELETE_REVERSE)
        {
            text =
                "on save, delete " + rev_dir +
                " exit in room " + std::to_string(p.to_room_vnum) +
                " to " + std::to_string(p.from_room_vnum) +
                " (reverse of " + std::to_string(p.from_room_vnum) +
                "." + from_dir + ")";
        }
        else
        {
            text =
                "on save, " + rev_dir +
                " exit in room " + std::to_string(p.to_room_vnum) +
                " will be: to " + std::to_string(p.final_vnum) +
                "  Key: " + std::to_string(p.final_key) +
                "  Keyword: " + keyword +
                "  Flags: " + flags;
        }

        auto lines = olc_format_line(
            first_line ? "pending" : "",
            text,
            OLC_COL_HELP,
            label_width,
            term_width
        );

        for (const auto& l : lines)
            out.push_back(l);

        first_line = false;
    }

    return out;
}

static std::vector<std::string> olc_format_exit_list_lines(
    ROOM_INDEX_DATA* room,
    size_t label_width,
    int term_width,
    bool show_help)
{
    std::vector<std::string> out;

    if (!room)
    {
        out.push_back(std::string(OLC_COL_LABEL) +
            std::string(label_width, ' ') +
            OLC_COL_RESET + " : none");
        return out;
    }

    const int num_w = 4;   // "#12:"
    const int dir_w = 10;  // east / somewhere / northeast
    const int to_w  = 6;   // room vnum
    const int key_w = 6;   // key vnum

    bool first_line = true;
    int exit_num = 0;

    for (EXIT_DATA* ex = room->first_exit; ex; ex = ex->next)
    {
        ++exit_num;

        std::string num = "#" + std::to_string(exit_num) + ":";
        std::string dir = exit_dir_label(ex->vdir);
        std::string to  = std::to_string(ex->vnum);
        std::string key = std::to_string(ex->key);

        std::string kw = (ex->keyword && ex->keyword[0]) ? ex->keyword : "-";
        std::string flags = flag_string(ex->exit_info, (char * const *)ex_flags);
        if (flags.empty())
            flags = "-";

        std::string tail =
            "Keywords: " + kw +
            "  Flags: " + flags;

        char prefix[MSL];
        snprintf(prefix, sizeof(prefix),
            "%s%-*s%s : %s%-*s%s %s%-*s%s to %s%-*s%s  Key: %s%-*s%s  ",
            OLC_COL_LABEL,
            (int)label_width,
            first_line ? "exit" : "",
            OLC_COL_RESET,
            OLC_COL_INT,
            num_w,
            num.c_str(),
            OLC_COL_RESET,
            OLC_COL_ENUM,
            dir_w,
            dir.c_str(),
            OLC_COL_RESET,
            OLC_COL_INT,
            to_w,
            to.c_str(),
            OLC_COL_RESET,
            OLC_COL_INT,
            key_w,
            key.c_str(),
            OLC_COL_RESET
        );

        int prefix_len = visible_length(prefix);
        int available = term_width - prefix_len;
        if (available < 20)
            available = 20;

        auto wrapped = olc_format_wrap_text(tail, available, prefix_len);

        if (wrapped.empty())
            wrapped.push_back(tail);

        out.push_back(std::string(prefix) +
            OLC_COL_STRING + wrapped[0] + OLC_COL_RESET);

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

    if (exit_num == 0)
    {
        char buf[MSL];
        snprintf(buf, sizeof(buf),
            "%s%-*s%s : %snone%s",
            OLC_COL_LABEL,
            (int)label_width,
            "exit",
            OLC_COL_RESET,
            OLC_COL_LIST,
            OLC_COL_RESET
        );
        out.push_back(buf);
    }

    if (show_help)
    {
        std::string help =
            "Use: olcshow exit <selector>  (#3, east, 2.east, ?, keyword)";
        auto help_lines = olc_format_line("",
            help,
            OLC_COL_HELP,
            (int)label_width,
            term_width);

        for (const auto& l : help_lines)
            out.push_back(l);
    }

    return out;
}

static std::string format_columns_for_field(
    const std::vector<std::string>& items,
    int term_width,
    int max_name_len)
{
    int gutter = max_name_len + 3; /* "name : " */
    int available_width = term_width - gutter;

    if (available_width < 20)
        available_width = 20;

    return olc_format_columns(items, available_width, 0);
}

static std::vector<std::string> format_multiline_block(
    const char* name,
    const std::string& value,
    const char* val_color,
    int max_name_len)
{
    std::vector<std::string> out;

    int label_width = max_name_len;
    char buf[MSL];

    std::istringstream iss(value);
    std::string line;
    bool first = true;

    while (std::getline(iss, line))
    {
        if (first)
        {
            snprintf(buf, sizeof(buf),
                "%s%-*s%s : %s%s%s",
                OLC_COL_LABEL,
                label_width,
                name,
                OLC_COL_RESET,
                val_color,
                line.c_str(),
                OLC_COL_RESET);
            first = false;
        }
        else
        {
            snprintf(buf, sizeof(buf),
                "%*s   %s%s%s",
                label_width,
                "",
                val_color,
                line.c_str(),
                OLC_COL_RESET);
        }

        out.push_back(buf);
    }

    if (out.empty())
    {
        snprintf(buf, sizeof(buf),
            "%s%-*s%s : %s%s%s",
            OLC_COL_LABEL,
            label_width,
            name,
            OLC_COL_RESET,
            val_color,
            "None",
            OLC_COL_RESET);
        out.push_back(buf);
    }

    return out;
}

static std::string olc_get_field_display_value(
    CHAR_DATA* ch,
    void* obj,
    const OlcField& f)
{
    if (f.contextual_getter)
        return f.contextual_getter(ch, obj);
    if (f.getter)
        return f.getter(obj);
    return "";
}

static void olc_append_lines(
    std::vector<std::string>& dst,
    const std::vector<std::string>& src)
{
    dst.insert(dst.end(), src.begin(), src.end());
}

// --------------------------------------------
// ROOM_OLC room_olc OlcOps functions
// --------------------------------------------
static void* room_olc_clone(const void* src)
{
    return olc_room_clone(static_cast<const ROOM_INDEX_DATA*>(src));
}

static void room_olc_free_clone(void* obj)
{
    olc_room_free(static_cast<ROOM_INDEX_DATA*>(obj));
}

static void room_olc_apply_changes(void* original, void* working)
{
    olc_room_apply_changes(
        static_cast<ROOM_INDEX_DATA*>(original),
        static_cast<ROOM_INDEX_DATA*>(working)
    );
}

static void room_olc_save(CHAR_DATA* ch, void* original)
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

static void room_olc_after_commit(CHAR_DATA* ch, void* original, void* /*working*/)
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

static void room_olc_after_revert(CHAR_DATA* ch, void* /*original*/, void* /*working*/)
{
    if (!ch || !ch->desc || !ch->desc->olc)
        return;

    ch->desc->olc->pending_exit_side_effects.clear();
}

static const OlcOps room_olc_ops =
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
    ed->keyword = STRALLOC((char*)keyword.c_str());
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

    dst->name = src->name ? STRALLOC(src->name) : nullptr;
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
        STRFREE(room->name);

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
        STRFREE(src->name);
    src->name = dst->name ? STRALLOC(dst->name) : nullptr;

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
    ch->dest_buf = ed;
    start_editing(ch, ed->description);

    return true;
}

static bool olc_room_parse_exit_create_options(
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
    if (!str_prefix(next.c_str(), "two"))
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

static void olc_room_apply_create_options_to_exit(EXIT_DATA* ex, const OlcExitCreateOptions& opts)
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

static void olc_room_apply_create_options_to_pending_reverse(
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

static OlcExitSelector olc_room_resolve_exit_selector(ROOM_INDEX_DATA* room, const std::string& token)
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

static void olc_room_relink_exits(ROOM_INDEX_DATA* room)
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

static EXIT_DATA* olc_room_find_reverse_exit_to_room(ROOM_INDEX_DATA* room, int dir, int to_vnum)
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

static void olc_room_init_pending_reverse_from_live(
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

static OlcPendingExitSideEffect* olc_room_get_or_create_pending_reverse_exit(
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

static void olc_room_queue_reverse_delete(
    CHAR_DATA* ch,
    ROOM_INDEX_DATA* from,
    EXIT_DATA* ex)
{
    OlcPendingExitSideEffect* p = olc_room_get_or_create_pending_reverse_exit(ch, from, ex);
    if (!p)
        return;

    p->type = OlcExitSideEffectType::DELETE_REVERSE;
}

static void olc_room_queue_reverse_upsert(
    CHAR_DATA* ch,
    ROOM_INDEX_DATA* from,
    EXIT_DATA* ex)
{
    OlcPendingExitSideEffect* p = olc_room_get_or_create_pending_reverse_exit(ch, from, ex);
    if (!p)
        return;

    p->type = OlcExitSideEffectType::UPSERT_REVERSE;
}

static void olc_room_apply_pending_reverse_exit_delete(const OlcPendingExitSideEffect& p)
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

static void olc_room_apply_pending_reverse_exit_upsert(const OlcPendingExitSideEffect& p)
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
    back->keyword = STRALLOC((char*)(p.final_keyword.empty() ? "" : p.final_keyword.c_str()));

    ex->rexit = back;
    back->rexit = ex;
}

static void olc_room_apply_pending_exit_side_effect(const OlcPendingExitSideEffect& p)
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

static const char* olc_room_match_exit_command(const std::string& input)
{
    static const char* cmds[] =
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
        if (!str_cmp(input.c_str(), cmds[i]))
            return cmds[i];
    }

    /* Otherwise, find unique prefix match */
    const char* match = nullptr;

    for (int i = 0; cmds[i] != nullptr; ++i)
    {
        if (!str_prefix(input.c_str(), cmds[i]))
        {
            if (match)
                return nullptr; /* ambiguous */
            match = cmds[i];
        }
    }

    return match;
}

static bool olc_room_exit_command_is_ambiguous(const std::string& input)
{
    static const char* cmds[] =
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
        if (!str_prefix(input.c_str(), cmds[i]))
            ++matches;
    }

    return matches > 1;
}

static void olc_room_show_ambiguous_exit_command(CHAR_DATA* ch, const std::string& input)
{
    static const char* cmds[] =
    {
        "delete",
        "flags",
        "key",
        "desc",
        "keyword",
        nullptr
    };

    send_to_char(("Ambiguous exit command: " + input + "\n").c_str(), ch);
    send_to_char("Matches:\n", ch);

    for (int i = 0; cmds[i] != nullptr; ++i)
    {
        if (!str_prefix(input.c_str(), cmds[i]))
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

static bool olc_room_handle_exit_delete(CHAR_DATA* ch, ROOM_INDEX_DATA* room, EXIT_DATA* ex, bool two_way)
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

static bool olc_room_handle_exit_create(
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

static bool olc_room_handle_exit_flags(CHAR_DATA* ch, EXIT_DATA* ex, const std::string& flags)
{
    if (flags.empty())
    {
        send_to_char("No flags specified.\n", ch);

        std::string col = olc_format_exit_flags_list();
        if (!col.empty())
        {
            send_to_char("Valid flags:\n", ch);
            send_to_char(col.c_str(), ch);
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

        int bit = get_exflag(const_cast<char*>(word.c_str()));
        if (bit < 0)
        {
            send_to_char(("Unknown flag: " + word + "\n").c_str(), ch);

            std::string col = olc_format_exit_flags_list();
            if (!col.empty())
            {
                send_to_char("Valid flags:\n", ch);
                send_to_char(col.c_str(), ch);
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

static bool olc_room_handle_exit_key(CHAR_DATA* ch, EXIT_DATA* ex, std::istringstream& iss)
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

static std::string olc_room_exit_remaining_args(const std::string& arg)
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

static bool olc_room_handle_exit_keyword(CHAR_DATA* ch, EXIT_DATA* ex)
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

static bool olc_room_handle_exit_desc(CHAR_DATA* ch, EXIT_DATA* ex)
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

static bool olc_room_handle_bexit_flags(CHAR_DATA* ch, ROOM_INDEX_DATA* room, EXIT_DATA* ex, const std::string& flags)
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
            send_to_char(col.c_str(), ch);
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

        int bit = get_exflag(const_cast<char*>(word.c_str()));
        if (bit < 0)
        {
            send_to_char(("Unknown flag: " + word + "\n").c_str(), ch);

            std::string col = olc_format_exit_flags_list();
            if (!col.empty())
            {
                send_to_char("Valid flags:\n", ch);
                send_to_char(col.c_str(), ch);
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

static bool olc_room_handle_bexit_key(CHAR_DATA* ch, ROOM_INDEX_DATA* room, EXIT_DATA* ex, std::istringstream& iss)
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

static bool olc_room_handle_bexit_keyword(CHAR_DATA* ch, ROOM_INDEX_DATA* room, EXIT_DATA* ex)
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

static bool olc_room_edit_exit_field_impl(CHAR_DATA* ch, ROOM_INDEX_DATA* room, bool reverse_default)
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
        bool two_way = reverse_default || (!opt.empty() && !str_prefix(opt.c_str(), "two"));

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
        bool two_way = reverse_default || (!opt.empty() && !str_prefix(opt.c_str(), "two"));
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
            return olc_room_handle_bexit_flags(ch, room, sel.ex, flags);

        return olc_room_handle_exit_flags(ch, sel.ex, flags);
    }

    if (!str_cmp(resolved, "key"))
    {
        if (reverse_default)
            return olc_room_handle_bexit_key(ch, room, sel.ex, iss);

        return olc_room_handle_exit_key(ch, sel.ex, iss);
    }

    if (!str_cmp(resolved, "desc"))
        return olc_room_handle_exit_desc(ch, sel.ex);

    if (!str_cmp(resolved, "keyword"))
    {
        if (reverse_default)
            return olc_room_handle_bexit_keyword(ch, room, sel.ex);

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

bool olc_room_is_field_name(const char* input)
{
    if (!input || input[0] == '\0')
        return false;

    const OlcSchema* schema = get_room_schema();

    if (olc_find_field_fuzzy(schema, input))
        return true;

    if (olc_field_name_is_ambiguous(schema, input))
        return true;

    return false;
}

static void olc_room_edit_help(CHAR_DATA* ch)
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

bool olc_room_edit_interpret(CHAR_DATA* ch, char* command, char* argument)
{

    //log_printf("OLC INLINE: command='%s'", command);

    if (!olc_room_in_edit_mode(ch) || !command || command[0] == '\0')
        return false;

    if (olc_is_direction_alias(command))
        return false;

    if (olc_command_matches(command, "help") || !str_cmp(command, "?"))
    {
        if (argument[0] == '\0')
            olc_room_edit_help(ch);
        else
        {
            char buf[MSL];
            SPRINTF(buf, "%s %s", "help", argument);
            do_redit2(ch, buf);
        }
        return true;
    }

    if (olc_command_matches(command, "stop"))
    {
        char arg[MIL] = {0};
        argument = one_argument(argument, arg);

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
        do_olcshow(ch, argument);
        return true;
    }

    if (olc_command_matches(command, "olcshow"))
    {
        do_olcshow(ch, argument);
        return true;
    }

    if (olc_command_matches(command, "look"))
    {
        auto sess = ch->desc->olc;
        olc_show_room_preview(ch,
            static_cast<ROOM_INDEX_DATA*>(sess->working_copy),
            argument ? argument : const_cast<char*>(""));
        return true;
    }

    if (olc_command_matches(command, "olcset"))
    {
        do_olcset(ch, argument);
        return true;
    }

    if (olc_room_is_field_name(command))
    {
        char buf[MSL];
        snprintf(buf, sizeof(buf), "%s %s", command, argument ? argument : "");
        do_olcset(ch, buf);
        return true;
    }

    return false;
}

// --------------------------------------------
// OBJECT_OLC object_olc OlcOps functions
// --------------------------------------------
static void* object_olc_clone(const void* src)
{
    return olc_object_clone(static_cast<const OBJ_DATA*>(src));
}

static void object_olc_free_clone(void* obj)
{
    olc_object_free_clone(static_cast<OBJ_DATA*>(obj));
}

static void object_olc_apply_changes(void* original, void* working)
{
    OBJ_DATA* live = static_cast<OBJ_DATA*>(original);
    OBJ_DATA* draft = static_cast<OBJ_DATA*>(working);

    if (!live || !draft)
        return;

    olc_object_apply_instance_changes(live, draft);
}


static void object_olc_save(CHAR_DATA* ch, void* original)
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

static void object_olc_after_commit(CHAR_DATA* ch, void* original, void* working)
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

static void object_olc_after_revert(CHAR_DATA* ch, void* original, void* working)
{
    /* nothing needed yet */
}


static const OlcOps object_olc_ops =
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
static bool olc_object_edit_revert(CHAR_DATA* ch)
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

static void olc_object_show_header(CHAR_DATA* ch, OBJ_DATA* obj)
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
    ed->keyword = STRALLOC((char*)keyword.c_str());
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
    ch->dest_buf = ed;
    start_editing(ch, ed->description);
    return true;
}

static AFFECT_DATA* olc_find_object_affect_by_number(OBJ_DATA* obj, int index)
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

static std::string olc_object_affect_location_name(int loc)
{
    if (loc >= 0 && loc < MAX_APPLY_TYPE)
        return a_types[loc].name;

    return std::to_string(loc);
}

static std::vector<std::string> olc_object_format_affect_lines(
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
            "  Bitvector: " + std::to_string(af->bitvector);

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

static bool olc_object_parse_affect_value(
    CHAR_DATA* ch,
    int loc,
    const std::string& value_text,
    int& out_value)
{
    if (loc == APPLY_AFFECT)
    {
        out_value = get_aflag(const_cast<char*>(value_text.c_str()));
        if (out_value < 0)
        {
            ch_printf(ch, "Unknown affect flag: %s\n", value_text.c_str());
            return false;
        }
        return true;
    }

    if (loc > APPLY_AFFECT && loc < APPLY_WEAPONSPELL)
    {
        out_value = get_risflag(const_cast<char*>(value_text.c_str()));
        if (out_value < 0)
        {
            ch_printf(ch, "Unknown resistance flag: %s\n", value_text.c_str());
            return false;
        }
        return true;
    }

    return olc_format_parse_strict_int(value_text, out_value);
}

static bool olc_object_values_equal_display(const OBJ_DATA* obj, const OBJ_INDEX_DATA* proto)
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

static bool olc_object_extradescs_equal_proto(const OBJ_DATA* obj, const OBJ_INDEX_DATA* proto)
{
    if (!obj || !proto)
        return false;

    return olc_extradescs_equal(obj->first_extradesc, proto->first_extradesc);
}

static std::string olc_object_proto_extradesc_list_summary(const OBJ_INDEX_DATA* obj)
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

static std::vector<std::string> olc_object_format_proto_affect_lines(
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
            "  Bitvector: " + std::to_string(af->bitvector);

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

static std::string olc_object_values_summary(const OBJ_DATA* obj)
{
    char buf[MSL];

    if (!obj)
        return "none";

    snprintf(buf, sizeof(buf), "[%d] [%d] [%d] [%d] [%d] [%d]",
        obj->value[0], obj->value[1], obj->value[2],
        obj->value[3], obj->value[4], obj->value[5]);

    return buf;
}

static std::string olc_object_proto_values_summary(const OBJ_INDEX_DATA* obj)
{
    char buf[MSL];

    if (!obj)
        return "none";

    snprintf(buf, sizeof(buf), "[%d] [%d] [%d] [%d] [%d] [%d]",
        obj->value[0], obj->value[1], obj->value[2],
        obj->value[3], obj->value[4], obj->value[5]);

    return buf;
}

static const OlcItemValueInfo* olc_object_find_value_info(int item_type)
{
    extern const OlcItemValueInfo g_olc_item_value_info[];

    for (int i = 0; g_olc_item_value_info[i].item_type != -1; ++i)
    {
        if (g_olc_item_value_info[i].item_type == item_type)
            return &g_olc_item_value_info[i];
    }

    return nullptr;
}

static std::vector<std::string> olc_object_labeled_value_pairs(const OBJ_DATA* obj)
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

static std::vector<std::string> olc_object_proto_labeled_value_pairs(const OBJ_INDEX_DATA* obj)
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
    if (!ch || !ch->desc || !ch->desc->olc || !obj)
        return false;

    std::string arg = ch->desc->olc->last_cmd_arg;
    if (arg.empty())
    {
        olc_show_object_affect_help(ch);
        return false;
    }

    std::istringstream iss(arg);
    std::string first;
    iss >> first;

    if (first.empty())
    {
        olc_show_object_affect_help(ch);
        return false;
    }

    if (!str_cmp(first.c_str(), "add"))
    {
        std::string field_name;
        std::string value_text;
        int loc;
        int value = 0;

        iss >> field_name;
        std::getline(iss, value_text);
        value_text = olc_format_trim_left(value_text);

        if (field_name.empty() || value_text.empty())
        {
            olc_show_object_affect_help(ch);
            return false;
        }

        loc = get_atype(const_cast<char*>(field_name.c_str()));
        if (loc < 1)
        {
            ch_printf(ch, "Unknown affect field: %s\n", field_name.c_str());
            return false;
        }

        if (!olc_object_parse_affect_value(ch, loc, value_text, value))
        {
            if (!(loc > APPLY_AFFECT && loc < APPLY_WEAPONSPELL) && loc != APPLY_AFFECT)
                send_to_char("Affect value must be numeric.\n", ch);
            return false;
        }

        AFFECT_DATA* af = new AFFECT_DATA{};
        af->type = -1;
        af->duration = -1;
        af->location = loc;
        af->modifier = value;
        af->bitvector = -1;

        af->prev = obj->last_affect;
        af->next = nullptr;

        if (obj->last_affect)
            obj->last_affect->next = af;
        else
            obj->first_affect = af;

        obj->last_affect = af;

        ch->desc->olc->dirty = true;
        send_to_char("Affect added.\n", ch);
        return true;
    }

    if (first[0] != '#')
    {
        olc_show_object_affect_help(ch);
        return false;
    }

    int index = atoi(first.c_str() + 1);
    AFFECT_DATA* af = olc_find_object_affect_by_number(obj, index);
    if (!af)
    {
        send_to_char("No such affect.\n", ch);
        return false;
    }

    std::string cmd;
    iss >> cmd;

    if (cmd.empty())
    {
        olc_show_object_affect_help(ch);
        return false;
    }

    if (!str_cmp(cmd.c_str(), "delete"))
    {
        if (af->prev)
            af->prev->next = af->next;
        else
            obj->first_affect = af->next;

        if (af->next)
            af->next->prev = af->prev;
        else
            obj->last_affect = af->prev;

        delete af;
        ch->desc->olc->dirty = true;
        send_to_char("Affect removed.\n", ch);
        return true;
    }

    if (!str_cmp(cmd.c_str(), "location"))
    {
        std::string field_name;
        iss >> field_name;

        if (field_name.empty())
        {
            send_to_char("Set location to which affect field?\n", ch);
            return false;
        }

        int loc = get_atype(const_cast<char*>(field_name.c_str()));
        if (loc < 1)
        {
            ch_printf(ch, "Unknown affect field: %s\n", field_name.c_str());
            return false;
        }

        af->location = loc;
        ch->desc->olc->dirty = true;
        send_to_char("Affect location updated.\n", ch);
        return true;
    }

    if (!str_cmp(cmd.c_str(), "modifier"))
    {
        std::string value_text;
        std::getline(iss, value_text);
        value_text = olc_format_trim_left(value_text);

        int value = 0;
        if (value_text.empty())
        {
            send_to_char("Set modifier to what?\n", ch);
            return false;
        }

        if (!olc_object_parse_affect_value(ch, af->location, value_text, value))
        {
            if (!(af->location > APPLY_AFFECT && af->location < APPLY_WEAPONSPELL) &&
                af->location != APPLY_AFFECT)
                send_to_char("Affect modifier must be numeric.\n", ch);
            return false;
        }

        af->modifier = value;
        ch->desc->olc->dirty = true;
        send_to_char("Affect modifier updated.\n", ch);
        return true;
    }

    if (!str_cmp(cmd.c_str(), "duration"))
    {
        int value = 0;
        if (!(iss >> value))
        {
            send_to_char("Set duration to what?\n", ch);
            return false;
        }

        af->duration = value;
        ch->desc->olc->dirty = true;
        send_to_char("Affect duration updated.\n", ch);
        return true;
    }

    if (!str_cmp(cmd.c_str(), "type"))
    {
        int value = 0;
        if (!(iss >> value))
        {
            send_to_char("Set type to what?\n", ch);
            return false;
        }

        af->type = value;
        ch->desc->olc->dirty = true;
        send_to_char("Affect type updated.\n", ch);
        return true;
    }

    if (!str_cmp(cmd.c_str(), "bitvector"))
    {
        int value = 0;
        if (!(iss >> value))
        {
            send_to_char("Set bitvector to what?\n", ch);
            return false;
        }

        af->bitvector = value;
        ch->desc->olc->dirty = true;
        send_to_char("Affect bitvector updated.\n", ch);
        return true;
    }

    olc_show_object_affect_help(ch);
    return false;
}

static bool olc_object_in_edit_mode(CHAR_DATA* ch)
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

static bool olc_object_is_field_name(const char* command)
{
    if (!command || command[0] == '\0')
        return false;

    return olc_find_field_fuzzy(get_object_schema(), command) != nullptr;
}

static void olc_object_edit_help(CHAR_DATA* ch)
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

bool olc_object_edit_interpret(CHAR_DATA* ch, char* command, char* argument)
{
    if (!olc_object_in_inline_mode(ch) || !command || command[0] == '\0')
        return false;

    if (olc_is_direction_alias(command))
        return false;

    if (olc_command_matches(command, "help") || !str_cmp(command, "?"))
    {
        
        if (argument[0] == '\0')
            olc_object_edit_help(ch);
        else
        {
            char buf[MSL];
            SPRINTF(buf, "%s %s", "help", argument);
            do_oedit(ch, buf);
        }
        return true;
    }

    if (olc_command_matches(command, "stop"))
    {
        char arg[MIL] = {0};
        argument = one_argument(argument, arg);

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
        do_olcshow(ch, argument);
        return true;
    }

    if (olc_command_matches(command, "olcshow"))
    {
        do_olcshow(ch, argument);
        return true;
    }

    if (olc_command_matches(command, "look"))
    {
        return false;
    }

    if (olc_command_matches(command, "olcset"))
    {
        do_olcset(ch, argument);
        return true;
    }

    if (olc_object_is_field_name(command))
    {
        char buf[MSL];
        snprintf(buf, sizeof(buf), "%s %s", command, argument ? argument : "");
        do_olcset(ch, buf);
        return true;
    }

    return false;
}


// --------------------------------------------
// MOBILE_OLC mobile_olc OlcOps functions
// --------------------------------------------
static void* mobile_olc_clone(const void* src)
{
    return olc_mobile_clone(static_cast<const CHAR_DATA*>(src));
}

static void mobile_olc_free_clone(void* obj)
{
    olc_mobile_free(static_cast<CHAR_DATA*>(obj));
}

static void mobile_olc_apply_changes(void* original, void* working)
{
    olc_mobile_apply_instance_changes(
        static_cast<CHAR_DATA*>(original),
        static_cast<const CHAR_DATA*>(working));
}

static void mobile_olc_save(CHAR_DATA* ch, void* original)
{
    CHAR_DATA* mob = static_cast<CHAR_DATA*>(original);

    if (!ch || !mob)
        return;

    if (IS_NPC(mob) && BV_IS_SET(mob->act, ACT_PROTOTYPE))
        send_to_char("Prototype-backed mobile changes staged. Save area after commit if needed.\n", ch);
    else
        send_to_char("Live mobile changes saved to instance only.\n", ch);
}

static void mobile_olc_after_commit(CHAR_DATA* ch, void* /*original*/, void* /*working*/)
{
    /*
     * IMPORTANT:
     * Prototype delta sync is NOT done here.
     * It must be done in the main commit path where baseline is available.
     */
    if (ch)
        send_to_char("Mobile committed.\n", ch);
}

static void mobile_olc_after_revert(CHAR_DATA* ch, void* /*original*/, void* /*working*/)
{
    if (ch)
        send_to_char("Mobile reverted to pre-edit state.\n", ch);
}


static const OlcOps mobile_olc_ops =
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

// --------------------------------------------
// OLC_MOBILE olc_mobile specific functions
// --------------------------------------------


extern const flag_name npc_race[];
extern const flag_name act_flags[];
extern const flag_name aff_flags[];
extern const flag_name vip_flag_table[];

static bool olc_mobile_set_spec(void* obj, const std::string& value)
{
    auto mob = static_cast<CHAR_DATA*>(obj);
    if (!mob)
        return false;

    if (!str_cmp(value.c_str(), "none"))
    {
        mob->spec_fun = nullptr;
        return true;
    }

    SPEC_FUN* spec = spec_lookup(value.c_str());
    if (!spec)
        return false;

    mob->spec_fun = spec;
    return true;
}

static bool olc_mobile_set_spec2(void* obj, const std::string& value)
{
    auto mob = static_cast<CHAR_DATA*>(obj);
    if (!mob)
        return false;

    if (!str_cmp(value.c_str(), "none"))
    {
        mob->spec_2 = nullptr;
        return true;
    }

    SPEC_FUN* spec = spec_lookup(value.c_str());
    if (!spec)
        return false;

    mob->spec_2 = spec;
    return true;
}

static bool olc_mobile_set_ris_field(void* obj, int CHAR_DATA::*member, const std::string& value)
{
    auto mob = static_cast<CHAR_DATA*>(obj);
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

        int flag = get_risflag(const_cast<char*>(token.c_str()));
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

static std::string olc_mobile_get_ris_field(void* obj, int CHAR_DATA::*member)
{
    auto mob = static_cast<CHAR_DATA*>(obj);
    if (!mob)
        return "";

    return flag_string(mob->*member, ris_flags);
}

static bool olc_mobile_set_legacy_bits_field(void* obj, int CHAR_DATA::*member,
                                          const std::string& value,
                                          const char* const* table)
{
    auto mob = static_cast<CHAR_DATA*>(obj);
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
            if (!str_cmp(token.c_str(), table[i]))
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

static std::string olc_mobile_get_legacy_bits_field(void* obj,
                                                 int CHAR_DATA::*member,
                                                 char* const* table)
{
    auto mob = static_cast<CHAR_DATA*>(obj);
    if (!mob)
        return "";

    return flag_string(mob->*member, table);
}

static bool olc_mobile_set_pending_proto_die_field(CHAR_DATA* ch,
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


static std::string olc_mobile_get_pending_proto_die_field(CHAR_DATA* ch,
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

static bool olc_mobile_apply_pending_prototype_changes(CHAR_DATA* ch, CHAR_DATA* live)
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

static void olc_mobile_init_pending_proto_from_live(OlcSession* sess, CHAR_DATA* mob)
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

static void olc_mobile_reset_pending_proto(OlcSession* sess)
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

static bool olc_mobile_set_long_desc(char*& field, const std::string& value)
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

static bool olc_mobile_set_level(void* obj, const std::string& value)
{
    auto mob = static_cast<CHAR_DATA*>(obj);
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

static bool olc_mobile_set_race(void* obj, const std::string& value)
{
    auto mob = static_cast<CHAR_DATA*>(obj);
    int race = get_npc_race((char*)value.c_str());

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

static bool olc_mobile_set_speaking(void* obj, const std::string& value)
{
    auto mob = static_cast<CHAR_DATA*>(obj);
    int lang = get_langflag(const_cast<char*>(value.c_str()));

    if (!mob)
        return false;

    if (lang == LANG_UNKNOWN)
        return false;

    mob->speaking = lang;
    return true;
}

static bool olc_mobile_set_long_field(void* obj, const std::string& value)
{
    auto mob = static_cast<CHAR_DATA*>(obj);
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
    dst->pIndexData = src->pIndexData;

    dst->name        = src->name        ? STRALLOC(src->name)        : nullptr;
    dst->short_descr = src->short_descr ? STRALLOC(src->short_descr) : nullptr;
    dst->long_descr  = src->long_descr  ? STRALLOC(src->long_descr)  : nullptr;
    dst->description = src->description ? STRALLOC(src->description) : nullptr;

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

static bool olc_mobile_edit_revert(CHAR_DATA* ch)
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

static bool olc_mobile_in_edit_mode(CHAR_DATA* ch)
{
    return ch && ch->desc && ch->desc->olc &&
           ch->desc->olc->schema &&
           ch->desc->olc->schema->name &&
           !str_cmp(ch->desc->olc->schema->name, "mobile");
}

static void olc_mobile_edit_help(CHAR_DATA* ch)
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

bool olc_mobile_edit_interpret(CHAR_DATA* ch, char* command, char* argument)
{
    if (!olc_mobile_in_edit_mode(ch) || !command || command[0] == '\0')
        return false;

    if (olc_command_matches(command, "help") || !str_cmp(command, "?"))
    {
        char buf[MSL];
        SPRINTF(buf, "%s %s", "help", argument);
        do_medit(ch, buf);
        return true;
    }

    if (olc_command_matches(command, "stop"))
    {
        char arg[MIL] = {0};
        argument = one_argument(argument, arg);

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
        do_olcshow(ch, argument);
        return true;
    }

    if (olc_command_matches(command, "olcshow"))
    {
        do_olcshow(ch, argument);
        return true;
    }

    if (olc_command_matches(command, "look"))
        return false;

    if (olc_command_matches(command, "olcset"))
    {
        do_olcset(ch, argument);
        return true;
    }

    if (olc_find_field_fuzzy(ch->desc->olc->schema, command))
    {
        char buf[MSL];
        snprintf(buf, sizeof(buf), "%s %s", command, argument ? argument : "");
        do_olcset(ch, buf);
        return true;
    }

    return false;
}

static bool olc_mobile_parse_affect_value(
    CHAR_DATA* ch,
    int loc,
    const std::string& value_text,
    int& out_value)
{
    if (loc == APPLY_AFFECT)
    {
        out_value = get_aflag(const_cast<char*>(value_text.c_str()));
        if (out_value < 0)
        {
            ch_printf(ch, "Unknown affect flag: %s\n", value_text.c_str());
            return false;
        }
        return true;
    }

    if (loc > APPLY_AFFECT && loc < APPLY_WEAPONSPELL)
    {
        out_value = get_risflag(const_cast<char*>(value_text.c_str()));
        if (out_value < 0)
        {
            ch_printf(ch, "Unknown resistance flag: %s\n", value_text.c_str());
            return false;
        }
        return true;
    }

    return olc_format_parse_strict_int(value_text, out_value);
}

static AFFECT_DATA* olc_mobile_find_affect_by_number(CHAR_DATA* mob, int index)
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

static std::vector<std::string> olc_mobile_format_affect_lines(
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
            "  Bitvector: " + std::to_string(af->bitvector);

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
    if (!ch || !ch->desc || !ch->desc->olc || !mob)
        return false;

    std::string arg = ch->desc->olc->last_cmd_arg;
    if (arg.empty())
    {
        olc_show_mobile_affect_help(ch);
        return false;
    }

    std::istringstream iss(arg);
    std::string first;
    iss >> first;

    if (first.empty())
    {
        olc_show_mobile_affect_help(ch);
        return false;
    }

    if (!str_cmp(first.c_str(), "add"))
    {
        std::string field_name;
        std::string value_text;
        int loc;
        int value = 0;

        iss >> field_name;
        std::getline(iss, value_text);
        value_text = olc_format_trim_left(value_text);

        if (field_name.empty() || value_text.empty())
        {
            olc_show_mobile_affect_help(ch);
            return false;
        }

        loc = get_atype(const_cast<char*>(field_name.c_str()));
        if (loc < 1)
        {
            ch_printf(ch, "Unknown affect field: %s\n", field_name.c_str());
            return false;
        }

        if (!olc_mobile_parse_affect_value(ch, loc, value_text, value))
        {
            if (!(loc > APPLY_AFFECT && loc < APPLY_WEAPONSPELL) &&
                loc != APPLY_AFFECT)
                send_to_char("Affect value must be numeric.\n", ch);
            return false;
        }

        AFFECT_DATA* af = new AFFECT_DATA{};
        af->type = -1;
        af->duration = -1;
        af->location = loc;
        af->modifier = value;
        af->bitvector = -1;

        af->prev = mob->last_affect;
        af->next = nullptr;

        if (mob->last_affect)
            mob->last_affect->next = af;
        else
            mob->first_affect = af;

        mob->last_affect = af;

        ch->desc->olc->dirty = true;
        send_to_char("Affect added.\n", ch);
        return true;
    }

    if (first[0] != '#')
    {
        olc_show_mobile_affect_help(ch);
        return false;
    }

    int index = atoi(first.c_str() + 1);
    AFFECT_DATA* af = olc_mobile_find_affect_by_number(mob, index);
    if (!af)
    {
        send_to_char("No such affect.\n", ch);
        return false;
    }

    std::string cmd;
    iss >> cmd;
    if (cmd.empty())
    {
        olc_show_mobile_affect_help(ch);
        return false;
    }

    if (!str_cmp(cmd.c_str(), "delete"))
    {
        if (af->prev)
            af->prev->next = af->next;
        else
            mob->first_affect = af->next;

        if (af->next)
            af->next->prev = af->prev;
        else
            mob->last_affect = af->prev;

        delete af;
        ch->desc->olc->dirty = true;
        send_to_char("Affect removed.\n", ch);
        return true;
    }

    if (!str_cmp(cmd.c_str(), "location"))
    {
        std::string field_name;
        iss >> field_name;

        if (field_name.empty())
        {
            send_to_char("Set location to which affect field?\n", ch);
            return false;
        }

        int loc = get_atype(const_cast<char*>(field_name.c_str()));
        if (loc < 1)
        {
            ch_printf(ch, "Unknown affect field: %s\n", field_name.c_str());
            return false;
        }

        af->location = loc;
        ch->desc->olc->dirty = true;
        send_to_char("Affect location updated.\n", ch);
        return true;
    }

    if (!str_cmp(cmd.c_str(), "modifier"))
    {
        std::string value_text;
        std::getline(iss, value_text);
        value_text = olc_format_trim_left(value_text);

        int value = 0;
        if (value_text.empty())
        {
            send_to_char("Set modifier to what?\n", ch);
            return false;
        }

        if (!olc_mobile_parse_affect_value(ch, af->location, value_text, value))
        {
            if (!(af->location > APPLY_AFFECT && af->location < APPLY_WEAPONSPELL) &&
                af->location != APPLY_AFFECT)
                send_to_char("Affect modifier must be numeric.\n", ch);
            return false;
        }

        af->modifier = value;
        ch->desc->olc->dirty = true;
        send_to_char("Affect modifier updated.\n", ch);
        return true;
    }

    if (!str_cmp(cmd.c_str(), "duration"))
    {
        int value = 0;
        if (!(iss >> value))
        {
            send_to_char("Set duration to what?\n", ch);
            return false;
        }

        af->duration = value;
        ch->desc->olc->dirty = true;
        send_to_char("Affect duration updated.\n", ch);
        return true;
    }

    if (!str_cmp(cmd.c_str(), "type"))
    {
        int value = 0;
        if (!(iss >> value))
        {
            send_to_char("Set type to what?\n", ch);
            return false;
        }

        af->type = value;
        ch->desc->olc->dirty = true;
        send_to_char("Affect type updated.\n", ch);
        return true;
    }

    if (!str_cmp(cmd.c_str(), "bitvector"))
    {
        int value = 0;
        if (!(iss >> value))
        {
            send_to_char("Set bitvector to what?\n", ch);
            return false;
        }

        af->bitvector = value;
        ch->desc->olc->dirty = true;
        send_to_char("Affect bitvector updated.\n", ch);
        return true;
    }

    olc_show_mobile_affect_help(ch);
    return false;
}

// --------------------------------------------
// OLC_GENERIC olc_generic functions
// --------------------------------------------

static bool olc_affects_equal(const AFFECT_DATA* a, const AFFECT_DATA* b)
{
    while (a && b)
    {
        if (a->type      != b->type
         || a->duration  != b->duration
         || a->location  != b->location
         || a->modifier  != b->modifier
         || a->bitvector != b->bitvector)
            return false;

        a = a->next;
        b = b->next;
    }

    return (a == nullptr && b == nullptr);
}

static bool olc_extradescs_equal(const EXTRA_DESCR_DATA* a, const EXTRA_DESCR_DATA* b)
{
    while (a && b)
    {
        if (!olc_str_eq(a->keyword, b->keyword)
         || !olc_str_eq(a->description, b->description))
            return false;

        a = a->next;
        b = b->next;
    }

    return (a == nullptr && b == nullptr);
}

AFFECT_DATA* olc_clone_affects(AFFECT_DATA* src)
{
    AFFECT_DATA* first = nullptr;
    AFFECT_DATA* last = nullptr;

    for (; src; src = src->next)
    {
        AFFECT_DATA* af = new AFFECT_DATA{};

        af->type = src->type;
        af->duration = src->duration;
        af->location = src->location;
        af->modifier = src->modifier;
        af->bitvector = src->bitvector;

        af->prev = last;
        af->next = nullptr;

        if (!first)
            first = af;
        else
            last->next = af;

        last = af;
    }

    return first;
}

void olc_free_affects(AFFECT_DATA* af)
{
    while (af)
    {
        AFFECT_DATA* next = af->next;
        delete af;
        af = next;
    }
}

static void olc_apply_flag_delta(BitSet& dst, const BitSet& edited, const BitSet& baseline)
{
    for (size_t bit = 0; bit < edited.size(); ++bit)
    {
        bool was_set = baseline.test(bit);
        bool is_set  = edited.test(bit);

        if (!was_set && is_set)
            dst.set(bit);
        else if (was_set && !is_set)
            dst.clear(bit);
    }
}

bool olc_try_inline_interpret(CHAR_DATA* ch, char* command, char* argument, OlcInterpretStage stage)
{
    if (!ch || !ch->desc || !ch->desc->olc)
        return false;

    auto sess = ch->desc->olc;
    if (!sess->ops || !sess->ops->inline_interpret)
        return false;

    if (sess->ops->inline_interpret_stage != stage)
        return false;

    return sess->ops->inline_interpret(ch, command, argument);
}

static CMDTYPE* olc_find_command_exact_for_char(CHAR_DATA* ch, const char* command)
{
    CMDTYPE* cmd;

    if (!ch || !command || command[0] == '\0')
        return nullptr;

    for (cmd = command_hash[LOWER(command[0]) % 126]; cmd; cmd = cmd->next)
    {
        if (str_cmp(command, cmd->name))
            continue;

        if (command_is_authorized_for_char(ch, cmd))
            return cmd;

        return nullptr;
    }

    return nullptr;
}

static bool olc_can_use_command(CHAR_DATA* ch, const char* command)
{
    return olc_find_command_exact_for_char(ch, command) != nullptr;
}

bool olc_commit_current(CHAR_DATA* ch)
{
    if (!ch || !ch->desc || !ch->desc->olc)
        return false;

    auto sess = ch->desc->olc;
    if (!sess->working_copy || !sess->original || !sess->original_clone ||
        !sess->ops || !sess->ops->apply_changes ||
        !sess->ops->clone || !sess->ops->free_clone)
        return false;

    bool has_pending_mob_proto =
           sess->pending_mob_proto.hitnodice_set
        || sess->pending_mob_proto.hitsizedice_set
        || sess->pending_mob_proto.damnodice_set
        || sess->pending_mob_proto.damsizedice_set;

    if (!sess->dirty &&
        sess->pending_exit_side_effects.empty() &&
        !has_pending_mob_proto)
        return false;

    /*
     * 1) Apply working copy back to the live object.
     */
    sess->ops->apply_changes(sess->original, sess->working_copy);

    /*
     * 2) Mobile-specific prototype propagation.
     *    This MUST happen here, because only here do we have:
     *      original + working_copy + original_clone
     */
    bool prototype_changed = false;

    if (sess->schema && sess->schema->name &&
        !str_cmp(sess->schema->name, "mobile"))
    {
        CHAR_DATA* live     = static_cast<CHAR_DATA*>(sess->original);
        CHAR_DATA* edited   = static_cast<CHAR_DATA*>(sess->working_copy);
        CHAR_DATA* baseline = static_cast<CHAR_DATA*>(sess->original_clone);

        prototype_changed |= olc_mobile_maybe_sync_prototype(live, baseline, edited);
        prototype_changed |= olc_mobile_apply_pending_prototype_changes(ch, live);
        /*
        if (prototype_changed && live && live->pIndexData && live->pIndexData->area)
            fold_area(live->pIndexData->area,
                      live->pIndexData->area->filename,
                      FALSE);
        */
    }

    /*
     * 3) Existing post-commit hook.
     */
    if (sess->ops->after_commit)
        sess->ops->after_commit(ch, sess->original, sess->working_copy);

    /*
     * 4) Refresh the baseline snapshot so REVERT goes back to the
     *    just-committed state, not the pre-session state.
     */
    void* new_baseline = sess->ops->clone(sess->original);
    if (!new_baseline)
    {
        send_to_char("Commit applied, but baseline refresh failed.\n", ch);
        return false;
    }

    void* old_baseline = sess->original_clone;
    sess->original_clone = new_baseline;

    if (old_baseline)
        sess->ops->free_clone(old_baseline);

    /*
     * 5) For mobile OLC, clear and re-init pending prototype-only fields
     *    from the now-committed live/index state.
     */
    if (sess->schema && sess->schema->name &&
        !str_cmp(sess->schema->name, "mobile"))
    {
        olc_mobile_reset_pending_proto(sess);
        olc_mobile_init_pending_proto_from_live(
            sess,
            static_cast<CHAR_DATA*>(sess->original));
    }

    sess->dirty = false;
    return true;
}

void olc_discard_current_working_copy(CHAR_DATA* ch)
{
    if (!ch || !ch->desc || !ch->desc->olc)
        return;

    auto sess = ch->desc->olc;
    if (!sess->ops || !sess->ops->free_clone)
        return;

    if (sess->working_copy)
    {
        sess->ops->free_clone(sess->working_copy);
        sess->working_copy = nullptr;
    }

    if (sess->original_clone)
    {
        sess->ops->free_clone(sess->original_clone);
        sess->original_clone = nullptr;
    }

    sess->pending_exit_side_effects.clear();
    sess->last_cmd_arg.clear();
    sess->dirty = false;
    olc_mobile_reset_pending_proto(sess);
}

void olc_send_value_error(CHAR_DATA* ch, const OlcField& f,
                         const std::string& value,
                         int term_width, int indent)
{
    switch (f.value_type)
    {
        case OlcValueType::INT:
        {
            int dummy = 0;

            if (!olc_format_parse_strict_int(value, dummy))
            {
                send_to_char("Value must be a number.\n", ch);
            }
            else
            {
                char buf[MSL];

                if (f.min_int != INT_MIN || f.max_int != INT_MAX)
                {
                    snprintf(buf, sizeof(buf),
                        "Value out of range. Acceptable range is %d to %d.\n",
                        f.min_int, f.max_int);
                    send_to_char(buf, ch);
                }
                else
                {
                    send_to_char("Invalid numeric value.\n", ch);
                }
            }
            break;
        }

        case OlcValueType::BOOL:
            send_to_char("Value must be true/false (yes/no, 1/0).\n", ch);
            break;

        case OlcValueType::ENUM:
        {
            send_to_char("Invalid value.\n", ch);

            auto suggestions = olc_enum_suggestions(value, f);

            if (!suggestions.empty())
            {
                send_to_char("Did you mean:\n", ch);
                std::string col = olc_format_columns(suggestions, term_width, indent);
                send_to_char(col.c_str(), ch);
            }
            else
            {
                auto values = olc_enum_values_vec(f);
                if (!values.empty())
                {
                    send_to_char("Valid values:\n", ch);
                    std::string col = olc_format_columns(values, term_width, indent);
                    send_to_char(col.c_str(), ch);
                }
            }
            break;
        }

        case OlcValueType::FLAG:
        {
            send_to_char("Invalid flag value.\n", ch);

            auto values = olc_enum_values_vec(f);
            if (!values.empty())
            {
                send_to_char("Valid flags:\n", ch);
                std::string col = olc_format_columns(values, term_width, indent);
                send_to_char(col.c_str(), ch);
            }
            break;
        }

        case OlcValueType::STRING:
            send_to_char("Invalid text value.\n", ch);
            break;

        case OlcValueType::EDITOR:
            send_to_char("This field must be edited, not set directly.\n", ch);
            break;

        default:
            send_to_char("Invalid value.\n", ch);
            break;
    }
}

static bool olc_finish_editor_substate(CHAR_DATA* ch)
{
    if (!ch || !ch->desc || !ch->desc->olc)
        return false;

    auto sess = ch->desc->olc;
    if (!sess->schema || !sess->working_copy)
        return false;

    if (ch->substate <= SUB_NONE)
        return false;

    for (const auto& f : *sess->schema->fields)
    {
        if (f.value_type != OlcValueType::EDITOR)
            continue;

        if (f.editor_substate != ch->substate)
            continue;

        if (!f.editor_setter)
            continue;

        std::string text = copy_buffer(ch);
        stop_editing(ch);

        ch->substate = SUB_NONE;
        ch->dest_buf = nullptr;

        bool ok = f.editor_setter(ch, sess->working_copy, text);
        if (ok)
        {
            sess->dirty = true;
            ch_printf(ch, "%s updated.\n\r", f.name);
        }
        else
        {
            ch_printf(ch, "%s failed to update.\n\r", f.name);
        }
        return true;
    }

    return false;
}

static bool olc_has_active_session(CHAR_DATA* ch)
{
    return ch && ch->desc && ch->desc->olc;
}

static bool olc_matches_word(const std::string& input, const char* word)
{
    return !str_prefix(input.c_str(), word);
}

static bool olc_dispatch_entry_command(CHAR_DATA* ch, const std::string& type, const std::string& rest)
{
    char buf[MSL];

    if (olc_matches_word(type, "room") || olc_matches_word(type, "r"))
    {
        if (!olc_can_use_command(ch, "redit2"))
        {
            send_to_char("You do not have permission to edit rooms (redit2)\n", ch);
            return true;
        }

        do_redit2(ch, "");
        return true;
    }

    if (olc_matches_word(type, "object") || olc_matches_word(type, "obj") || olc_matches_word(type, "o"))
    {
        if (!olc_can_use_command(ch, "oedit"))
        {
            send_to_char("You do not have permission to edit objects (oedit)\n", ch);
            return true;
        }

        if (rest.empty())
        {
            send_to_char("Usage: olc object <object>\n", ch);
            return true;
        }

        snprintf(buf, sizeof(buf), "%s", rest.c_str());
        do_oedit(ch, buf);
        return true;
    }

    if (olc_matches_word(type, "mobile") || olc_matches_word(type, "mob") || olc_matches_word(type, "m"))
    {
        if (!olc_can_use_command(ch, "medit"))
        {
            send_to_char("You do not have permission to edit mobs (medit)\n", ch);
            return true;
        }

        if (rest.empty())
        {
            send_to_char("Usage: olc mobile <mobile>\n", ch);
            return true;
        }

        snprintf(buf, sizeof(buf), "%s", rest.c_str());
        do_medit(ch, buf);
        return true;
    }

    return false;
}

static bool olc_dispatch_session_command(CHAR_DATA* ch, const std::string& cmd, const std::string& rest)
{
    if (!olc_has_active_session(ch))
        return false;

    auto sess = ch->desc->olc;

    if (olc_matches_word(cmd, "show"))
    {
        char arg1[MIL];
        char arg2[MIL];
        char restbuf[MSL];

        arg1[0] = '\0';
        arg2[0] = '\0';
        restbuf[0] = '\0';

        snprintf(restbuf, sizeof(restbuf), "%s", rest.c_str());
        char* p = restbuf;
        p = one_argument(p, arg1);
        p = one_argument(p, arg2);

        std::string field = arg1[0] != '\0' ? arg1 : "";
        std::string value = p && p[0] != '\0' ? p : "";

        if (!value.empty())
            value.erase(0, value.find_first_not_of(' '));

        if (field.empty() && arg2[0] != '\0')
            field = arg2;

        olc_show(ch, field, value);
        return true;
    }

    if (olc_matches_word(cmd, "help") || cmd == "?")
    {
        std::string field = rest;
        if (!field.empty())
            field.erase(0, field.find_first_not_of(' '));

        if (field.empty())
            olc_show(ch, "help", "");
        else
            olc_show(ch, field, "help");
        return true;
    }

    if (olc_matches_word(cmd, "set"))
    {
        if (rest.empty())
        {
            send_to_char("Usage: olc set <field> <value>\n", ch);
            return true;
        }

        char buf[MSL];
        snprintf(buf, sizeof(buf), "%s", rest.c_str());
        do_olcset(ch, buf);
        return true;
    }

    if (olc_matches_word(cmd, "commit"))
    {
        if (olc_commit_current(ch))
            send_to_char("Current changes committed.\n", ch);
        else
            send_to_char("No pending changes to commit.\n", ch);
        return true;
    }

    if (olc_matches_word(cmd, "revert"))
    {
        if (!sess->schema || !sess->schema->name)
        {
            send_to_char("No active OLC schema.\n", ch);
            return true;
        }

        if (!str_cmp(sess->schema->name, "room"))
        {
            olc_room_edit_revert(ch);
            return true;
        }

        if (!str_cmp(sess->schema->name, "object"))
        {
            olc_object_edit_revert(ch);
            return true;
        }

        if (!str_cmp(sess->schema->name, "mobile"))
        {
            olc_mobile_edit_revert(ch);
            return true;
        }

        send_to_char("Revert is not supported for this editor.\n", ch);
        return true;
    }

    if (olc_matches_word(cmd, "save") || olc_matches_word(cmd, "done"))
    {
        olc_stop(ch, true);
        return true;
    }

    if (olc_matches_word(cmd, "abort") || olc_matches_word(cmd, "cancel"))
    {
        olc_stop(ch, false);
        return true;
    }

    /*
     * Builder-friendly shortcut:
     * While editing, allow "olc <field> <value>" to behave like olcset.
     */
    if (olc_find_field_fuzzy(sess->schema, cmd))
    {
        char buf[MSL];
        if (!rest.empty())
            snprintf(buf, sizeof(buf), "%s %s", cmd.c_str(), rest.c_str());
        else
            snprintf(buf, sizeof(buf), "%s", cmd.c_str());

        do_olcset(ch, buf);
        return true;
    }

    return false;
}

// --------------------------------------------
// OLC_SHOW olc_show functions
// --------------------------------------------
// --------------------------------------------
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
            capitalize( dir_name[pexit->vdir] ) );
        }
        else if ( IS_SET(pexit->exit_info, EX_WINDOW) )
        {
            STRAPP( buf, "%-5s - (window)",
            capitalize( dir_name[pexit->vdir] ) );
        }
        else if ( IS_SET(pexit->exit_info, EX_xAUTO) )
        {
        STRAPP( buf, "%-5s - %s",
            capitalize( pexit->keyword ),
            room_is_dark( pexit->to_room )
            ?  "Too dark to tell"
            : pexit->to_room->name );
        }
        else
            STRAPP( buf, "%-5s - %s",
            capitalize( dir_name[pexit->vdir] ),
            room_is_dark( pexit->to_room )
            ?  "Too dark to tell"
            : pexit->to_room->name ); 
        ch_printf(ch, "%s\n", buf);
    }   
    buf[0] = '\0';         
    if ( !found )
        ch_printf(ch, "%s", "none.\n");
}

static void olc_show_room_exit_help(CHAR_DATA* ch)
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
        send_to_char(col.c_str(), ch);
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
                    send_to_char( bitset_to_string( room->room_flags, r_flags ).c_str(), ch );
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
    const char* ed = get_extra_descr(arg.c_str(), room->first_extradesc);
    if (ed)
    {
        out = ed;
        if (!out.empty())
        {
            send_to_char("\n", ch);
            send_to_char(out.c_str(), ch);
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
        send_to_char((l + "\n").c_str(), ch);
}

// ------------------------
// ------ OBJECTS ---------
// ------------------------

static void olc_show_object_compare_line(
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

static void olc_show_object_string_compare_block(
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
        send_to_char(l.c_str(), ch);
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
        send_to_char(l.c_str(), ch);
        send_to_char("\n", ch);
    }
}

static void olc_show_object_compare_block(
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
        send_to_char(l.c_str(), ch);
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
        send_to_char(l.c_str(), ch);
        send_to_char("\n", ch);
    }
}

static void olc_show_object_extradesc_help(CHAR_DATA* ch)
{
    send_to_char("Usage:\n", ch);
    send_to_char("  extradesc <keyword>     (edit/create)\n", ch);
    send_to_char("  extradesc -<keyword>    (delete)\n", ch);
}

static void olc_show_object_value_meanings_block(
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

static void olc_show_object_affect_help(CHAR_DATA* ch)
{
    send_to_char("Usage:\n", ch);
    send_to_char("  affect add <field> <value>\n", ch);
    send_to_char("  affect #<n> delete\n", ch);
    send_to_char("  affect #<n> location <field>\n", ch);
    send_to_char("  affect #<n> modifier <value>\n", ch);
    send_to_char("  affect #<n> duration <value>\n", ch);
    send_to_char("  affect #<n> type <value>\n", ch);
    send_to_char("  affect #<n> bitvector <value>\n", ch);
}

static void olc_show_object(CHAR_DATA* ch, OBJ_DATA* obj, bool help_only_mode, int term_width)
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
            send_to_char(l.c_str(), ch);
            send_to_char("\n", ch);
        }
    }

    /* Instance-only: keep standalone */
    {
        auto lines = olc_format_line("Wear Loc", get_int_field(obj->wear_loc), OLC_COL_INT, label_width, term_width);
        for (const auto& l : lines)
        {
            send_to_char(l.c_str(), ch);
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
            send_to_char(l.c_str(), ch);
            send_to_char("\n", ch);
        }
    }
    else
    {
        auto lines = olc_format_line("Affects", "none", OLC_COL_LIST, label_width, term_width);
        for (const auto& l : lines)
        {
            send_to_char(l.c_str(), ch);
            send_to_char("\n", ch);
        }
    }
    if (proto && !olc_affects_equal(obj->first_affect, proto->first_affect))
    {
        auto plines = olc_object_format_proto_affect_lines(proto, label_width, term_width);
        for (const auto& l : plines)
        {
            send_to_char(l.c_str(), ch);
            send_to_char("\n", ch);
        }
    }    
}

// ------------------------
// -------MOBILES----------
// ------------------------
static void olc_show_mobile_affect_help(CHAR_DATA* ch)
{
    send_to_char("Usage:\n", ch);
    send_to_char("  affect add <field> <value>\n", ch);
    send_to_char("  affect #<n> delete\n", ch);
    send_to_char("  affect #<n> location <field>\n", ch);
    send_to_char("  affect #<n> modifier <value>\n", ch);
    send_to_char("  affect #<n> duration <value>\n", ch);
    send_to_char("  affect #<n> type <value>\n", ch);
    send_to_char("  affect #<n> bitvector <value>\n", ch);
}

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
    std::string res_flags_str   = flag_string(mob->resistant, ris_flags);
    std::string imm_flags_str   = flag_string(mob->immune, ris_flags);
    std::string sus_flags_str   = flag_string(mob->susceptible, ris_flags);
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
        send_to_char((line + "\n").c_str(), ch);

    auto aff_lines = olc_format_line(
        "affected",
        aff_flags_str.empty() ? "none" : aff_flags_str,
        OLC_COL_LIST,
        10,
        term_width
    );
    for (const auto& line : aff_lines)
        send_to_char((line + "\n").c_str(), ch);

    auto vip_lines = olc_format_line(
        "vip",
        vip_flags_str.empty() ? "none" : vip_flags_str,
        OLC_COL_LIST,
        10,
        term_width
    );
    for (const auto& line : vip_lines)
        send_to_char((line + "\n").c_str(), ch);

    auto part_lines = olc_format_line(
        "body parts",
        part_flags_str.empty() ? "none" : part_flags_str,
        OLC_COL_LIST,
        10,
        term_width
    );
    for (const auto& line : part_lines)
        send_to_char((line + "\n").c_str(), ch);        

    auto res_lines = olc_format_line(
        "resistant",
        res_flags_str.empty() ? "none" : res_flags_str,
        OLC_COL_LIST,
        10,
        term_width
    );
    for (const auto& line : res_lines)
        send_to_char((line + "\n").c_str(), ch);        

    auto imm_lines = olc_format_line(
        "immune",
        imm_flags_str.empty() ? "none" : imm_flags_str,
        OLC_COL_LIST,
        10,
        term_width
    );
    for (const auto& line : imm_lines)
        send_to_char((line + "\n").c_str(), ch);    

    auto sus_lines = olc_format_line(
        "suscept",
        sus_flags_str.empty() ? "none" : sus_flags_str,
        OLC_COL_LIST,
        10,
        term_width
    );
    for (const auto& line : sus_lines)
        send_to_char((line + "\n").c_str(), ch);   

    auto att_lines = olc_format_line(
        "attacks",
        att_flags_str.empty() ? "none" : att_flags_str,
        OLC_COL_LIST,
        10,
        term_width
    );
    for (const auto& line : att_lines)
        send_to_char((line + "\n").c_str(), ch);           

    auto def_lines = olc_format_line(
        "defenses",
        def_flags_str.empty() ? "none" : def_flags_str,
        OLC_COL_LIST,
        10,
        term_width
    );
    for (const auto& line : def_lines)
        send_to_char((line + "\n").c_str(), ch);          

    if (mob->first_affect)
    {
        auto lines = olc_mobile_format_affect_lines(mob, 10, term_width);
        for (const auto& l : lines)
        {
            send_to_char(l.c_str(), ch);
            send_to_char("\n", ch);
        }
    }
    else
    {
        auto lines = olc_format_line("Affects", "none", OLC_COL_LIST, 10, term_width);
        for (const auto& l : lines)
        {
            send_to_char(l.c_str(), ch);
            send_to_char("\n", ch);
        }
    }

}

// ------------------------
// -------GENERIC----------
// ------------------------
static void olc_show_extradesc_help(CHAR_DATA* ch)
{
    send_to_char("Usage:\n", ch);
    send_to_char("  extradesc <keyword>     (edit/create)\n", ch);
    send_to_char("  extradesc -<keyword>    (delete)\n", ch);
}

static bool olc_show_handle_extradesc_list(
    CHAR_DATA* ch,
    void* working_copy,
    const OlcField& f,
    ROOM_INDEX_DATA* room,
    OBJ_DATA* obj,
    const std::string& value,
    size_t max_name_len,
    int term_width,
    std::vector<std::string>& other_lines)
{
    if (f.meta_type != OlcMetaType::EXTRA_DESC_LIST)
        return false;

    if (!room && !obj)
        return false;

    EXTRA_DESCR_DATA* first_extradesc = room ? room->first_extradesc : obj->first_extradesc;

    if (!value.empty())
    {
        EXTRA_DESCR_DATA* ed = nullptr;

        for (ed = first_extradesc; ed; ed = ed->next)
        {
            if (nifty_is_name_prefix((char*)value.c_str(), ed->keyword))
                break;
        }

        if (!ed)
        {
            send_to_char("No such extra description.\n", ch);
            return true;
        }

        send_to_char(ed->description ? ed->description : "(no description)\n", ch);
        return true;
    }

    std::string summary = olc_get_field_display_value(ch, working_copy, f);

    if (summary.empty() && room)
        summary = "none";

    olc_append_lines(
        other_lines,
        olc_format_line(
            f.name,
            summary,
            OLC_COL_LIST,
            (int)max_name_len,
            term_width));

    return true;
}

static bool olc_show_handle_exit_list(
    CHAR_DATA* ch,
    const OlcField& f,
    ROOM_INDEX_DATA* room,
    size_t max_name_len,
    int term_width,
    std::vector<std::string>& other_lines)
{
    if (f.meta_type != OlcMetaType::EXIT_LIST)
        return false;

    if (!room)
        return false;

    olc_append_lines(
        other_lines,
        olc_format_exit_list_lines(
            room,
            max_name_len,
            term_width,
            false));

    olc_append_lines(
        other_lines,
        olc_format_pending_exit_side_effects(
            ch,
            max_name_len,
            term_width));

    return true;
}

static bool olc_show_handle_meta_list(
    const OlcField& f,
    OBJ_DATA* obj,
    CHAR_DATA* mob,
    size_t max_name_len,
    int term_width,
    std::vector<std::string>& other_lines)
{
    if (f.meta_type == OlcMetaType::OBJ_AFFECT_LIST)
    {
        if (!obj)
            return false;

        olc_append_lines(
            other_lines,
            olc_object_format_affect_lines(
                obj,
                max_name_len,
                term_width));
        return true;
    }

    if (f.meta_type == OlcMetaType::MOB_AFFECT_LIST)
    {
        if (!mob)
            return false;

        olc_append_lines(
            other_lines,
            olc_mobile_format_affect_lines(
                mob,
                max_name_len,
                term_width));
        return true;
    }

    return false;
}

static bool olc_show_handle_special_list_field(
    CHAR_DATA* ch,
    void* working_copy,
    const OlcField& f,
    ROOM_INDEX_DATA* room,
    OBJ_DATA* obj,
    CHAR_DATA* mob,
    const std::string& value,
    size_t max_name_len,
    int term_width,
    std::vector<std::string>& other_lines)
{
    if (olc_show_handle_extradesc_list(
            ch, working_copy, f, room, obj, value, max_name_len, term_width, other_lines))
        return true;

    if (olc_show_handle_exit_list(
            ch, f, room, max_name_len, term_width, other_lines))
        return true;

    if (olc_show_handle_meta_list(
            f, obj, mob, max_name_len, term_width, other_lines))
        return true;

    return false;
}

static void olc_show_main_help(CHAR_DATA* ch)
{
    send_to_char("OLC commands:\n", ch);
    send_to_char("  olc room              - edit current room\n", ch);
    send_to_char("  olc object <target>   - edit object by keyword/vnum\n", ch);
    send_to_char("  olc mobile <target>   - edit mobile by keyword/vnum\n", ch);
    send_to_char("\n", ch);
    send_to_char("While editing:\n", ch);
    send_to_char("  olc show [field] [value]\n", ch);
    send_to_char("  olc help [field]\n", ch);
    send_to_char("  olc set <field> <value>\n", ch);
    send_to_char("  olc commit\n", ch);
    send_to_char("  olc revert\n", ch);
    send_to_char("  olc save      (same as stop save)\n", ch);
    send_to_char("  olc abort     (same as stop abort)\n", ch);
    send_to_char("  olc done      (same as save)\n", ch);
}

static void olc_show_help(CHAR_DATA* ch, const std::string& field)
{
    auto d = ch->desc;
    if (!d || !d->olc)
    {
        send_to_char("You have not chosen anything to edit.\n", ch);
        return;
    }

    int term_width = 75;
    if (d->naws_enabled && d->term_width > 20)
        term_width = d->term_width;

    auto sess = d->olc;
    bool found = false;

    ROOM_INDEX_DATA* room = nullptr;
    OBJ_DATA* obj = nullptr;
    CHAR_DATA* mob = nullptr;

    if (!str_cmp(sess->schema->name, "room"))
        room = static_cast<ROOM_INDEX_DATA*>(sess->working_copy);
    else if (!str_cmp(sess->schema->name, "object"))
        obj = static_cast<OBJ_DATA*>(sess->working_copy);
    else if (!str_cmp(sess->schema->name, "mobile"))
        mob = static_cast<CHAR_DATA*>(sess->working_copy);
    if (!room && !obj && !mob)
        log_printf("Unknown olc session in olc_show_help");
    /*
     * Help header
     */
    if (field.empty() || !str_cmp(field.c_str(), "help"))
    {
        char buf[MSL];

        if (obj)
        {
            olc_object_show_header(ch, obj);
        }
        else if (mob)
        {
            snprintf(buf, sizeof(buf),
                "%s[Mobile Editor Help]%s\n",
                OLC_COL_HEADER,
                OLC_COL_RESET
            );
            send_to_char(buf, ch);
        }
        else
        {
            snprintf(buf, sizeof(buf),
                "%s[Room Editor Help]%s\n",
                OLC_COL_HEADER,
                OLC_COL_RESET
            );
            send_to_char(buf, ch);
        }
    }

    std::vector<std::string> lines;
    size_t max_name_len = 0;

    for (const auto& f : *sess->schema->fields)
    {
        if (!field.empty() && str_cmp(field.c_str(), "help") &&
            str_prefix_utf8(field.c_str(), f.name))
            continue;

        size_t len = strlen(f.name);
        if (len > max_name_len)
            max_name_len = len;
    }

    for (const auto& f : *sess->schema->fields)
    {
        if (!field.empty() && str_cmp(field.c_str(), "help") &&
            str_prefix_utf8(field.c_str(), f.name))
            continue;

        found = true;

        std::string help_text = f.help ? f.help : "";

        auto help_lines = olc_format_line(
            f.name,
            help_text,
            OLC_COL_HELP,
            (int)max_name_len,
            term_width
        );

        for (const auto& l : help_lines)
            lines.push_back(l);

        if (f.value_type == OlcValueType::ENUM || f.value_type == OlcValueType::FLAG)
        {
            auto vals = olc_enum_values_vec(f);
            if (!vals.empty())
            {
                std::string cols = format_columns_for_field( vals, term_width, (int)max_name_len );
                auto value_lines = format_multiline_block( "", cols, OLC_COL_HELP, (int)max_name_len );
                for (const auto& l : value_lines)
                    lines.push_back(l);
            }
        }
    }

    for (const auto& l : lines)
        send_to_char((l + "\n").c_str(), ch);

    if (!found)
        send_to_char("Unknown field.\n", ch);
}

void olc_show(CHAR_DATA* ch, const std::string& field, const std::string& value)
{
    auto d = ch->desc;
    if (!d || !d->olc)
    {
        send_to_char("You have not chosen anything to edit.", ch);
        return;
    }

    bool show_help = (value == "help") || (field == "help");
    int term_width = 75;

    if (d->naws_enabled && d->term_width > 20)
        term_width = d->term_width;

    auto sess = d->olc;
    bool found = false;

    ROOM_INDEX_DATA* room = nullptr;
    OBJ_DATA* obj = nullptr;
    CHAR_DATA* mob = nullptr;

    if (!str_cmp(sess->schema->name, "room"))
        room = static_cast<ROOM_INDEX_DATA*>(sess->working_copy);
    else if (!str_cmp(sess->schema->name, "object"))
        obj = static_cast<OBJ_DATA*>(sess->working_copy);
    else if (!str_cmp(sess->schema->name, "mobile"))
        mob = static_cast<CHAR_DATA*>(sess->working_copy);

    /*
     * Type-specific full display
     */
    if (field.empty() && !show_help)
    {
        if (room)
        {
            olc_show_room(ch, room, false, term_width);
            send_to_char("\n(Type 'olcshow help' to see field help and valid values)\n", ch);
            return;
        }

        if (obj)
        {
            olc_show_object(ch, obj, false, term_width);
            send_to_char("\n(Type 'olcshow help' to see field help and valid values)\n", ch);
            return;
        }

        if (mob)
        {
            olc_show_mob(ch, mob, false, term_width);
            send_to_char("\n(Type 'olcshow help' to see field help and valid values)\n", ch);
            return;
        }
    }

    /*
     * Help mode is now fully separate
     */
    if (show_help)
    {
        olc_show_help(ch, field);
        return;
    }
    // The rest of this function is purely generic catch all field handling
    std::vector<std::string> int_fields;
    std::vector<std::string> other_lines;

    size_t max_name_len = 0;

    for (const auto& f : *sess->schema->fields)
    {
        if (!field.empty() && str_prefix_utf8(field.c_str(), f.name))
            continue;

        size_t len = strlen(f.name);
        if (len > max_name_len)
            max_name_len = len;
    }

    for (const auto& f : *sess->schema->fields)
    {
        if (!field.empty() && str_prefix_utf8(field.c_str(), f.name))
            continue;

        found = true;

        if (olc_show_handle_special_list_field(
                ch,
                sess->working_copy,
                f,
                room,
                obj,
                mob,
                value,
                max_name_len,
                term_width,
                other_lines))
        {
            continue;
        }

        std::string display_value = olc_get_field_display_value(
            ch,
            sess->working_copy,
            f);

        if (f.value_type == OlcValueType::INT)
        {
            std::string entry = std::string(f.name) + "=" + display_value;
            int_fields.push_back(entry);
            continue;
        }

        const char* color = OLC_COL_STRING;
        switch (f.value_type)
        {
            case OlcValueType::INT:    color = OLC_COL_INT; break;
            case OlcValueType::ENUM:   color = OLC_COL_ENUM; break;
            case OlcValueType::FLAG:   color = OLC_COL_LIST; break;
            case OlcValueType::EDITOR: color = OLC_COL_STRING; break;
            case OlcValueType::STRING: color = OLC_COL_STRING; break;
            case OlcValueType::BOOL:   color = OLC_COL_ENUM; break;
            default:                   color = OLC_COL_STRING; break;
        }

        auto lines = olc_format_line(
            f.name,
            display_value,
            color,
            (int)max_name_len,
            term_width
        );

        for (const auto& l : lines)
            other_lines.push_back(l);
    }

    if (!int_fields.empty())
    {
        std::string prefix = std::string(OLC_COL_LABEL) + "Values" + OLC_COL_RESET + "      : ";
        int prefix_length = visible_length(prefix.c_str());
        int current_len = prefix_length;

        std::string line = prefix;

        for (size_t i = 0; i < int_fields.size(); ++i)
        {
            std::string part = std::string(OLC_COL_INT) + int_fields[i] + OLC_COL_RESET;

            if (i > 0)
                part = " &w| " + part;

            int part_length = visible_length(part.c_str());

            if ((int)(current_len + part_length) > term_width)
            {
                line += "\n";
                send_to_char(line.c_str(), ch);

                line = std::string(prefix_length, ' ') + part;
                current_len = prefix_length + part_length;
            }
            else
            {
                line += part;
                current_len += part_length;
            }
        }

        line += "\n";
        send_to_char(line.c_str(), ch);
    }

    for (const auto& l : other_lines)
        send_to_char((l + "\n").c_str(), ch);

    if (field.empty())
        send_to_char("\n(Type 'olcshow help' to see field help and valid values)\n", ch);

    if (!found)
        send_to_char("Unknown field.\n", ch);
}



// --------------------------------------------
// Session Control OLC_START OLC_STOP OLC_SET
// --------------------------------------------
void olc_start(CHAR_DATA* ch, void* target, const OlcSchema* schema, const OlcOps* ops)
{
    auto d = ch ? ch->desc : nullptr;
    if (!d)
        return;

    if (d->olc)
    {
        send_to_char("You are already in a session.\n", ch);
        return;
    }

    if (!target || !schema || !ops || !ops->clone || !ops->free_clone || !ops->apply_changes)
    {
        send_to_char("OLC is not configured for this target.\n", ch);
        return;
    }

    d->olc = new OlcSession();
    d->olc->original = target;
    d->olc->working_copy = ops->clone(target);
    d->olc->original_clone = ops->clone(target);
    d->olc->schema = schema;
    d->olc->ops = ops;
    d->olc->last_cmd_arg.clear();
    d->olc->dirty = false;
    d->olc->pending_exit_side_effects.clear();
    d->olc->mode = OlcEditMode::NONE;
    d->olc->anchor_room = nullptr;

    olc_mobile_reset_pending_proto(d->olc);
    if (schema && schema->name && !str_cmp(schema->name, "mobile"))
        olc_mobile_init_pending_proto_from_live(
            d->olc,
            static_cast<CHAR_DATA*>(target));    

    if (!d->olc->working_copy || !d->olc->original_clone)
    {
        if (d->olc->working_copy)
            ops->free_clone(d->olc->working_copy);
        if (d->olc->original_clone)
            ops->free_clone(d->olc->original_clone);

        delete d->olc;
        d->olc = nullptr;
        send_to_char("Failed to start OLC session.\n", ch);
        return;
    }
}

void olc_stop(CHAR_DATA* ch, bool save)
{
    auto d = ch ? ch->desc : nullptr;
    if (!d || !d->olc)
        return;

    auto sess = d->olc;

    if (save)
    {
        olc_commit_current(ch);

        if (sess->ops && sess->ops->save)
            sess->ops->save(ch, sess->original);

        send_to_char("Changes saved.\n", ch);
    }
    else
    {
        send_to_char("Changes discarded.\n", ch);
    }

    olc_discard_current_working_copy(ch);

    delete d->olc;
    d->olc = nullptr;
}

void olc_set(CHAR_DATA* ch, const std::string& field, const std::string& value)
{
    auto d = ch->desc;
    if (!d || !d->olc)
        return;

    auto sess = d->olc;

    int term_width = 80;

    if (d->naws_enabled && d->term_width > 20)
        term_width = d->term_width;
    int indent = 2;

    const OlcField* f = olc_find_field_fuzzy(sess->schema, field);

    if (!f)
    {
        if (olc_field_name_is_ambiguous(sess->schema, field))
        {
            send_to_char("Ambiguous field.\n", ch);

            auto matches = olc_ambiguous_field_matches(sess->schema, field);
            if (!matches.empty())
            {
                send_to_char("Matches:\n", ch);
                std::string col = olc_format_columns(matches, term_width, indent);
                send_to_char(col.c_str(), ch);
            }
        }
        else
        {
            send_to_char("Unknown field.\n", ch);

            auto suggestions = olc_field_suggestions(sess->schema, field);

            if (!suggestions.empty())
            {
                send_to_char("Did you mean:\n", ch);
                std::string col = olc_format_columns(suggestions, term_width, indent);
                send_to_char(col.c_str(), ch);
            }
            else
            {
                auto all = olc_all_field_names(sess->schema);

                send_to_char("Valid fields:\n", ch);
                std::string col = olc_format_columns(all, term_width, indent);
                send_to_char(col.c_str(), ch);
            }
        }

        return;
    }

    if (f->value_type == OlcValueType::EDITOR)
    {
//      bool iscommand = false;
        if (!f->editor_takes_argument && !value.empty())
        {
            send_to_char("Use without value to enter editor.\n", ch);
            return;
        }

        if (f->editor_takes_argument && value.empty())
        {
//            iscommand = true;
//            send_to_char("This field requires an argument.\n", ch);
//            return;
        }
        sess->last_cmd_arg = value;
        f->editor(ch, sess->working_copy);
        return;
    }

    if (value.empty())
    {
        send_to_char("No value provided.\n", ch);
        return;
    }

    bool ok = false;

    if (f->setter)
        ok = f->setter(sess->working_copy, value);
    else if (f->editor_setter)
        ok = f->editor_setter(ch, sess->working_copy, value);
    else
    {
        send_to_char("Field is not writable.\n", ch);
        return;
    }

    if (!ok)
    {
        olc_send_value_error(ch, *f, value, term_width, indent);
        return;
    }

    sess->dirty = true;
    send_to_char("Field updated.\n", ch);
}

// --------------------------------------------
// DO_COMMANDS DO_OLCSET DO_?EDIT DO_OLC
// --------------------------------------------
void do_olcset(CHAR_DATA* ch, char* argument)
{

    if (!ch || !ch->desc || !ch->desc->olc)
    {
        send_to_char("You are not editing anything.\n", ch);
        return;
    }

    if (ch->substate > SUB_NONE)
    {
        if (olc_finish_editor_substate(ch))
            return;
    }

    char field_buf[MIL] = {0};
    argument = one_argument(argument, field_buf);

    if (field_buf[0] == '\0')
    {
        send_to_char("Set what?\n", ch);
        olc_show(ch, "help", "");
        return;
    }

    std::string field(field_buf);
    std::string value(argument ? argument : "");

    // Trim leading spaces
    value.erase(0, value.find_first_not_of(' '));

    if (!str_cmp(field.c_str(), "bexit"))
    {
        olc_set(ch, "bexit", value);
        return;
    }

    olc_set(ch, field, value);
}

void do_olcshow(CHAR_DATA* ch, char* argument)
{
    char field_buf[MIL] = {0};
    argument = one_argument(argument, field_buf);

    std::string field(field_buf);
    std::string value(argument ? argument : "");

    value.erase(0, value.find_first_not_of(' '));

    olc_show(ch, field, value);
}

void do_redit2(CHAR_DATA* ch, char* argument)
{
    char arg[MIL] = {0};

    if (olc_room_in_edit_mode(ch))
    {
        if (!argument || argument[0] == '\0')
        {
            olc_room_edit_help(ch);
            return;
        }

        do_olc(ch, argument);
        return;
    }

    argument = one_argument(argument, arg);

    if (arg[0] == '\0' || !str_cmp(arg, "start"))
    {
        olc_room_edit_enter(ch);
        return;
    }

    if (!str_cmp(arg, "help") || !str_cmp(arg, "?"))
    {
        if (argument[0] != '\0')
            olc_show(ch, argument, "help");
        else
            olc_show(ch, arg, "");
        return;
    }

    if (!str_cmp(arg, "stop"))
    {
        char arg2[MIL] = {0};
        argument = one_argument(argument, arg2);

        if (!str_cmp(arg2, "save") || arg2[0] == '\0')
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
    char arg1[MIL];
    char arg2[MIL];

    if (!ch || !ch->desc)
        return;

    /* =========================
     * Existing object edit session commands
     * ========================= */
    if (olc_object_in_edit_mode(ch))
    {
        if (argument[0] == '\0')
        {
            olc_object_edit_help(ch);
            return;
        }
        do_olc(ch, argument);
        return;
    }

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    //Start a new object edit session
    OBJ_DATA* obj = NULL;
    if (arg1[0] == '\0')
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
    char arg1[MIL];
    char arg2[MIL];

    if (!ch || !ch->desc)
        return;

    /*
     * Existing mobile edit session commands
     */
    if (olc_mobile_in_edit_mode(ch))
    {
        if (argument[0] == '\0')
        {
            olc_mobile_edit_help(ch);
            return;
        }

        do_olc(ch, argument);
        return;
    }

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    /*
     * Start a new mobile edit session
     */
    CHAR_DATA* victim = nullptr;

    if (arg1[0] == '\0')
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

void do_olc(CHAR_DATA* ch, char* argument)
{
    char arg1[MIL];

    if (!ch || !ch->desc)
        return;

    argument = one_argument(argument, arg1);

    std::string cmd = arg1[0] != '\0' ? arg1 : "";
    std::string rest = argument && argument[0] != '\0' ? argument : "";

    if (!rest.empty())
        rest.erase(0, rest.find_first_not_of(' '));

    if (cmd.empty())
    {
        if (olc_has_active_session(ch))
        {
            olc_show(ch, "", "");
            return;
        }

        olc_show_main_help(ch);
        return;
    }

    if (olc_matches_word(cmd, "help") || cmd == "?")
    {
        if (olc_has_active_session(ch) && !rest.empty())
            olc_show(ch, rest, "help");
        else if (olc_has_active_session(ch))
            olc_show(ch, "help", "");
        else
            olc_show_main_help(ch);
        return;
    }

    /*
     * First try in-session commands.
     */
    if (olc_dispatch_session_command(ch, cmd, rest))
        return;

    /*
     * Then try opening an editor.
     * Existing do_?edit handlers retain their own permission/startup logic.
     */
    if (olc_dispatch_entry_command(ch, cmd, rest))
        return;

    send_to_char("Unknown OLC command. Type 'olc help'.\n", ch);
}