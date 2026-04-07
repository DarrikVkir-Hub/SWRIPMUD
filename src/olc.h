
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
* 			New OLC Schema-based System	Header File                        *
* ------------------------------------------------------------------------ *
* 	There are only two hooks currently for this system:                    *
* One in handler.c in char_to_room to handle rwalk                         *
* One in interpret() (two small blocks of code) to intercept OLC commands  *
* Not counting the do_declarations, obviously                              *
****************************************************************************/
#include <type_traits>

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
    // DO_COMMANDS DO_?EDIT

// New OLC definitions

template<typename T>
T* field_as(void* obj, size_t offset)
{
    return reinterpret_cast<T*>((char*)obj + offset);
}

template<typename T>
bool set_int_field(T& field, const std::string& value, int min = INT_MIN, int max = INT_MAX)
{
    try
    {
        int v = std::stoi(value);
        if (v < min || v > max)
            return false;

        field = v;
        return true;
    }
    catch (...)
    {
        return false;
    }
}

inline std::string get_int_field(int value)
{
    return std::to_string(value);
}

inline bool set_str_field(char*& field, const std::string& value)
{
    if (field)
        STRFREE(field);

    field = STRALLOC(const_cast<char*>(value.c_str()));
    return true;
}

inline std::string get_str_field(char* field)
{
    return field ? field : "";
}

inline bool set_enum_legacy_field(int& field, const std::string& value, const char* const* table)
{
    int val;
    if (!enum_from_string_legacy(value, val, table))
        return false;

    field = val;
    return true;
}

inline std::string get_enum_legacy_field(int field, const char* const* table)
{
    return enum_to_string_legacy(field, table);
}

inline std::string get_enum_flag_field(int field, const flag_name *table)
{
    return enum_to_string_flag(field, table);
}

inline bool set_enum_flag_field(int& field, const std::string& value, const flag_name *table)
{
    int val;
    if (!enum_from_string_flag(value, val, table))
        return false;

    field = val;
    return true;
}

inline bool set_flag_field(BitSet& bs, const std::string& value, const flag_name* table)
{
    return bitset_apply_from_string(bs, value, table);
}

inline std::string get_flag_field(const BitSet& bs, const flag_name* table)
{
    return bitset_to_string(bs, table);
}

inline bool bitset_apply_from_legacy_string(BitSet& bs, const std::string& input, const char* const* table)
{
    std::istringstream iss(input);
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

        int bit = -1;
        for (int i = 0; table && table[i] != nullptr; ++i)
        {
            if (!str_cmp_utf8(token.c_str(), table[i]))
            {
                bit = i;
                break;
            }
        }

        if (bit < 0)
            continue;

        switch (op)
        {
            case '-':
                bs.clear(bit);
                break;
            case '!':
                bs.toggle(bit);
                break;
            case '+':
            default:
                bs.set(bit);
                break;
        }

        changed = true;
    }

    return changed;
}

inline std::string get_legacy_flag_field(const BitSet& bs, const char* const* table)
{
    std::string out;

    if (!table)
        return out;

    for (int i = 0; table[i] != nullptr; ++i)
    {
        if (bs.test(i))
        {
            if (!out.empty())
                out += " ";
            out += table[i];
        }
    }

    return out.empty() ? "none" : out;
}

enum class OlcEditMode
{
    NONE,
    ROOM_INLINE,
    OBJECT_INLINE,
    MOBILE_INLINE,
};

struct OlcPendingMobPrototypeChanges
{
    bool hitnodice_set = false;
    sh_int hitnodice = 0;

    bool hitsizedice_set = false;
    sh_int hitsizedice = 0;

    bool damnodice_set = false;
    sh_int damnodice = 0;

    bool damsizedice_set = false;
    sh_int damsizedice = 0;
};

enum class OlcExitSideEffectType
{
    UPSERT_REVERSE,
    DELETE_REVERSE
};

struct OlcPendingExitSideEffect
{
    OlcExitSideEffectType type = OlcExitSideEffectType::UPSERT_REVERSE;

    int from_room_vnum = 0;
    int from_dir = -1;
    int to_room_vnum = 0;

    /* Desired final reverse-exit state on save */
    int final_vnum = 0;          /* usually from_room_vnum */
    int final_rvnum = 0;         /* usually to_room_vnum */
    int final_key = -1;
    int final_exit_info = 0;
    std::string final_keyword;

    bool initialized = false;
};

enum class OlcValueType
{
    INT,
    STRING,
    ENUM,
    FLAG,
    EDITOR,        // long text / editor-driven
    LIST,          // extradesc, etc
    BOOL,
};

enum class OlcMetaType
{
    NONE,
    FLAG_TABLE,     // BitSet (flag_name[])
    ENUM_FLAG,      // enum using flag_name[]
    ENUM_LEGACY,     // enum using const char*[]
    EXTRA_DESC_LIST,
    EXIT_LIST,
    OBJ_AFFECT_LIST,
    MOB_AFFECT_LIST,    
    LIST,
};

template <typename T>
struct OlcField
{
    const char* name;
    OlcValueType value_type;
    const void* meta = nullptr;
    OlcMetaType meta_type = OlcMetaType::NONE;

    std::function<bool(CHAR_DATA* ch, T* obj)> editor = nullptr;
    std::function<bool(T* obj, const std::string& value)> setter = nullptr;
    std::function<std::string(T* obj)> getter = nullptr;
    std::function<bool(CHAR_DATA* ch, T* obj, const std::string& value)> editor_setter = nullptr;
    std::function<std::string(CHAR_DATA* ch, T* obj)> contextual_getter = nullptr;

    int editor_substate = 0;

    const char* help = nullptr;
    int min_int = INT_MIN;
    int max_int = INT_MAX;
    bool editor_takes_argument = false;

    OlcField(
        const char* n,
        OlcValueType vt,
        const void* m,
        OlcMetaType mt,
        std::function<bool(CHAR_DATA* ch, T* obj)> e,
        std::function<bool(T* obj, const std::string&)> s,
        std::function<std::string(T* obj)> g,
        std::function<bool(CHAR_DATA* ch, T* obj, const std::string& value)> es,
        std::function<std::string(CHAR_DATA* ch, T* obj)> cg,
        int esub,
        const char* h = nullptr,
        int minv = INT_MIN,
        int maxv = INT_MAX,
        bool editor_arg = false
    )
        : name(n),
          value_type(vt),
          meta(m),
          meta_type(mt),
          editor(std::move(e)),
          setter(std::move(s)),
          getter(std::move(g)),
          editor_setter(std::move(es)),
          contextual_getter(std::move(cg)),
          editor_substate(esub),
          help(h),
          min_int(minv),
          max_int(maxv),
          editor_takes_argument(editor_arg)
    {}
};

template <typename T, typename M>
using olc_member_ptr = M T::*;

template <typename T, typename M>
OlcField<T> make_olc_int_field(
    const char* name,
    olc_member_ptr<T, M> member,
    const char* help,
    int minv = INT_MIN,
    int maxv = INT_MAX)
{
    static_assert(std::is_integral<M>::value || std::is_enum<M>::value,
                  "make_olc_int_field requires an integral or enum member");

    return OlcField<T>{
        name,
        OlcValueType::INT,
        nullptr,
        OlcMetaType::NONE,
        nullptr,
        [member, minv, maxv](T* obj, const std::string& value) -> bool
        {
            return set_int_field(obj->*member, value, minv, maxv);
        },
        [member](T* obj) -> std::string
        {
            return get_int_field(obj->*member);
        },
        nullptr,
        nullptr,
        0,
        help,
        minv,
        maxv,
        false
    };
}
template <typename T>
OlcField<T> make_olc_string_field(
    const char* name,
    olc_member_ptr<T, char*> member,
    const char* help)
{
    return OlcField<T>{
        name,
        OlcValueType::STRING,
        nullptr,
        OlcMetaType::NONE,
        nullptr,
        [member](T* obj, const std::string& value) -> bool
        {
            return set_str_field(obj->*member, value);
        },
        [member](T* obj) -> std::string
        {
            return get_str_field(obj->*member);
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
template <typename T, typename M>
OlcField<T> make_olc_enum_legacy_field(
    const char* name,
    olc_member_ptr<T, M> member,
    const char* const* table,
    const char* help)
{
    static_assert(std::is_integral<M>::value || std::is_enum<M>::value,
                  "make_olc_enum_legacy_field requires an integral or enum member");

    return OlcField<T>{
        name,
        OlcValueType::ENUM,
        (const char*)table,
        OlcMetaType::ENUM_LEGACY,
        nullptr,
        [member, table](T* obj, const std::string& value) -> bool
        {
            int temp = 0;
            if (!set_enum_legacy_field(temp, value, table))
                return false;

            obj->*member = static_cast<M>(temp);
            return true;
        },
        [member, table](T* obj) -> std::string
        {
            return get_enum_legacy_field(static_cast<int>(obj->*member), table);
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
template <typename T, typename M>
OlcField<T> make_olc_enum_flag_field(
    const char* name,
    olc_member_ptr<T, M> member,
    const flag_name* table,
    const char* help)
{
    static_assert(std::is_integral<M>::value || std::is_enum<M>::value,
                  "make_olc_enum_flag_field requires an integral or enum member");

    return OlcField<T>{
        name,
        OlcValueType::ENUM,
        (const void*)table,
        OlcMetaType::ENUM_FLAG,
        nullptr,
        [member, table](T* obj, const std::string& value) -> bool
        {
            int temp = 0;
            if (!set_enum_flag_field(temp, value, table))
                return false;

            obj->*member = static_cast<M>(temp);
            return true;
        },
        [member, table](T* obj) -> std::string
        {
            return get_enum_flag_field(static_cast<int>(obj->*member), table);
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
template <typename T, typename M>
OlcField<T> make_olc_flag_field(
    const char* name,
    olc_member_ptr<T, M> member,
    const flag_name* table,
    const char* help)
{
    return OlcField<T>{
        name,
        OlcValueType::FLAG,
        (const void*)table,
        OlcMetaType::FLAG_TABLE,
        nullptr,
        [member, table](T* obj, const std::string& value) -> bool
        {
            return set_flag_field(obj->*member, value, table);
        },
        [member, table](T* obj) -> std::string
        {
            return get_flag_field(obj->*member, table);
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
template <typename T>
OlcField<T> make_olc_editor_field(
    const char* name,
    olc_member_ptr<T, char*> member,
    int substate_val,
    const char* help)
{
    return OlcField<T>{
        name,
        OlcValueType::EDITOR,
        nullptr,
        OlcMetaType::NONE,
        [member, substate_val](CHAR_DATA* ch, T* obj) -> bool
        {
            ch->substate = substate_val;
            ch->dest_buf = obj;
            ch->last_cmd = do_olcset;
            start_editing(ch, obj->*member);
            return true;
        },
        nullptr,
        [member](T* obj) -> std::string
        {
            return (obj->*member) ? (obj->*member) : "";
        },
        [member](CHAR_DATA* /*ch*/, T* obj, const std::string& value) -> bool
        {
            return set_str_field(obj->*member, value);
        },
        nullptr,
        substate_val,
        help,
        INT_MIN,
        INT_MAX,
        false
    };
}
template <typename T>
OlcField<T> make_olc_custom_editor_field(
    const char* name,
    OlcMetaType meta_type,
    std::function<bool(CHAR_DATA* ch, T* obj)> editor,
    std::function<std::string(T* obj)> getter,
    int editor_substate,
    const char* help,
    bool editor_takes_argument = true)
{
    return OlcField<T>{
        name,
        OlcValueType::EDITOR,
        nullptr,
        meta_type,
        std::move(editor),
        nullptr,
        std::move(getter),
        nullptr,
        nullptr,
        editor_substate,
        help,
        INT_MIN,
        INT_MAX,
        editor_takes_argument
    };
}
template <typename T>
OlcField<T> make_olc_bool_field(
    const char* name,
    olc_member_ptr<T, bool> member,
    const char* help)
{
    return OlcField<T>{
        name,
        OlcValueType::BOOL,
        nullptr,
        OlcMetaType::NONE,
        nullptr,
        [member](T* obj, const std::string& value) -> bool
        {
            return set_bool_field(obj->*member, value);
        },
        [member](T* obj) -> std::string
        {
            return get_bool_field(obj->*member);
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
template <typename T>
OlcField<T> make_olc_legacy_flag_field(
    const char* name,
    olc_member_ptr<T, BitSet> member,
    const char* const* table,
    const char* help)
{
    return OlcField<T>{
        name,
        OlcValueType::FLAG,
        (const void*)table,
        OlcMetaType::ENUM_LEGACY,
        nullptr,
        [member, table](T* obj, const std::string& value) -> bool
        {
            return bitset_apply_from_legacy_string(obj->*member, value, table);
        },
        [member, table](T* obj) -> std::string
        {
            return get_legacy_flag_field(obj->*member, table);
        },
        nullptr,
        nullptr,
        0,
        help,
        INT_MIN,
        INT_MAX,
        false
    };
}template <typename T>
OlcField<T> make_olc_custom_value_field(
    const char* name,
    OlcValueType value_type,
    const void* meta,
    OlcMetaType meta_type,
    std::function<bool(T* obj, const std::string& value)> setter,
    std::function<std::string(T* obj)> getter,
    const char* help,
    int minv = INT_MIN,
    int maxv = INT_MAX)
{
    return OlcField<T>{
        name,
        value_type,
        meta,
        meta_type,
        nullptr,
        std::move(setter),
        std::move(getter),
        nullptr,
        nullptr,
        0,
        help,
        minv,
        maxv,
        false
    };
}
/*
template <typename T>
struct OlcSchema
{
    const char* name;
    const std::vector<OlcField<T>>* fields;
};
*/
struct OlcSchemaBase
{
    const char* name = nullptr;
};

template <typename T>
struct OlcSchema : public OlcSchemaBase
{
    const std::vector<OlcField<T>>* fields = nullptr;
};

enum class OlcInterpretStage
{
    EARLY,   /* before skill/social/auto-exit handling */
    LATE     /* after those checks, before "Huh?" */
};

struct OlcOps
{
    /* Core lifecycle */
    void* (*clone)(const void* src) = nullptr;
    void  (*free_clone)(void* obj) = nullptr;
    void  (*apply_changes)(void* original, void* working) = nullptr;
    void  (*save)(CHAR_DATA* ch, void* original) = nullptr;

    /* Optional hooks */
    void  (*after_commit)(CHAR_DATA* ch, void* original, void* working) = nullptr;
    void  (*after_revert)(CHAR_DATA* ch, void* original, void* working) = nullptr;

    /* Optional inline interpret hook */
    bool (*inline_interpret)(CHAR_DATA* ch, char* command, char* argument) = nullptr;
    OlcInterpretStage inline_interpret_stage = OlcInterpretStage::EARLY;    
};



struct OlcSession
{
    void* working_copy = nullptr;     // cloned object
    void* original = nullptr;         // live object
    void* original_clone = nullptr;   // revert snapshot
    const OlcSchemaBase* schema = nullptr;
    const OlcOps* ops = nullptr;
    std::string last_cmd_arg;

    bool dirty = false;

    std::vector<OlcPendingExitSideEffect> pending_exit_side_effects;
    OlcPendingMobPrototypeChanges pending_mob_proto;

    OlcEditMode mode = OlcEditMode::NONE;
    ROOM_INDEX_DATA* anchor_room = nullptr;   // room-inline feature only
};


#ifndef MIL
#define MIL MAX_INPUT_LENGTH
#endif
#ifndef MSL
#define MSL MAX_STRING_LENGTH
#endif

// --------------------------------------------
// OLC defines, enums, and structs
// --------------------------------------------

void olc_mobile_reset_pending_proto(OlcSession* sess);
void olc_mobile_init_pending_proto_from_live(OlcSession* sess, CHAR_DATA* mob);
bool olc_format_parse_strict_int(const std::string& s, int& out);
void olc_show_room(CHAR_DATA* ch, ROOM_INDEX_DATA* room, bool show_help, int term_width);
void olc_show_object(CHAR_DATA* ch, OBJ_DATA* obj, bool help_only_mode, int term_width);
void olc_show_mob(CHAR_DATA* ch, CHAR_DATA* mob, bool help_only_mode, int term_width);
std::vector<std::string> olc_wrap_value_pairs( const std::vector<std::string>& pairs, int width);
std::vector<std::string> olc_format_line( const char* name, const std::string& value, const char* val_color, int max_name_len, int term_width);
std::vector<std::string> olc_format_exit_list_lines( ROOM_INDEX_DATA* room, size_t label_width, int term_width, bool show_help);
std::vector<std::string> olc_format_pending_exit_side_effects( CHAR_DATA* ch, size_t label_width, int term_width);
AFFECT_DATA* olc_find_object_affect_by_number(OBJ_DATA* obj, int index);
std::vector<std::string> olc_object_format_single_affect_lines( AFFECT_DATA* paf, size_t max_name_len, int term_width);
void olc_append_lines( std::vector<std::string>& dst, const std::vector<std::string>& src);
std::vector<std::string> olc_mobile_format_single_affect_lines( AFFECT_DATA* paf, size_t max_name_len, int term_width);
AFFECT_DATA* olc_mobile_find_affect_by_number(CHAR_DATA* mob, int index);
std::vector<std::string> olc_object_format_affect_lines( OBJ_DATA* obj, size_t label_width, int term_width);
std::vector<std::string> olc_mobile_format_affect_lines( CHAR_DATA* mob, size_t label_width, int term_width);

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
// GENERIC template functions for field handling
// GENERICOLCSTRING generic string templates - generic functions but dealing with OLC variables
// --------------------------------------------
template <typename T>
std::vector<std::string> olc_all_field_names(const OlcSchema<T>* schema)
{
    std::vector<std::string> out;

    if (!schema || !schema->fields)
        return out;

    for (const auto& f : *schema->fields)
        out.push_back(f.name);

    return out;
}

template <typename T>
std::vector<std::string> olc_field_suggestions(const OlcSchema<T>* schema, const std::string& input)
{
    std::vector<std::string> matches;

    if (!schema || !schema->fields)
        return matches;

    for (const auto& f : *schema->fields)
    {
        if (str_prefix(input.c_str(), f.name) == false)
        {
            matches.push_back(f.name);
            continue;
        }

        if (strcasestr(f.name, input.c_str()))
            matches.push_back(f.name);
    }

    std::sort(matches.begin(), matches.end());
    return matches;
}

template <typename T>
std::vector<std::string> olc_ambiguous_field_matches(const OlcSchema<T>* schema, const std::string& input)
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

template <typename T>
bool olc_field_name_is_ambiguous(const OlcSchema<T>* schema, const std::string& input)
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

template <typename T>
static std::string olc_get_field_display_value_typed(
    CHAR_DATA* ch,
    T* obj,
    const OlcField<T>& f)
{
    if (f.contextual_getter)
        return f.contextual_getter(ch, obj);

    if (f.getter)
        return f.getter(obj);

    return "";
}

template <typename T>
std::vector<std::string> olc_enum_values_vec(const OlcField<T>& f)
{
    std::vector<std::string> out;

    switch (f.meta_type)
    {
        case OlcMetaType::ENUM_FLAG:
        {
            auto table = static_cast<const flag_name*>(f.meta);
            for (size_t i = 0; table && table[i].name; ++i)
                out.push_back(table[i].name);
            break;
        }

        case OlcMetaType::FLAG_TABLE:
        {
            auto table = static_cast<const flag_name*>(f.meta);
            for (size_t i = 0; table && table[i].name; ++i)
                out.push_back(table[i].name);
            break;
        }

        case OlcMetaType::ENUM_LEGACY:
        {
            auto table = static_cast<const char* const*>(f.meta);
            for (int i = 0; table && table[i]; ++i)
                out.push_back(table[i]);
            break;
        }

        default:
            break;
    }

    return out;
}

template <typename T>
std::vector<std::string> olc_enum_suggestions(const std::string& input, const OlcField<T>& f)
{
    std::vector<std::string> matches;
    auto values = olc_enum_values_vec(f);

    for (const auto& v : values)
    {
        if (str_prefix_utf8(input.c_str(), v.c_str()) == false)
            matches.push_back(v);
    }

    return matches;
}

template <typename T>
void olc_send_value_error(CHAR_DATA* ch, const OlcField<T>& f, const std::string& value, int term_width, int indent);


template <typename T>
std::vector<std::string> olc_all_field_names(const OlcSchema<T>* schema);

template <typename T>
std::vector<std::string> olc_field_suggestions(const OlcSchema<T>* schema, const std::string& input);

template <typename T>
const OlcField<T>*  olc_find_field_fuzzy(const OlcSchema<T>* schema, const std::string& input)
{
    const OlcField<T>* prefix_match = nullptr;

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



template <typename T>
std::vector<std::string> olc_ambiguous_field_matches(const OlcSchema<T>* schema, const std::string& input);

template <typename T>
bool olc_field_name_is_ambiguous(const OlcSchema<T>* schema, const std::string& input);

template <typename T>
static std::string olc_get_field_display_value_typed( CHAR_DATA* ch, T* obj, const OlcField<T>& f);

template <typename T>
static bool olc_schema_has_field_name(const OlcSchema<T>* schema, const std::string& cmd);


// --------------------------------------------
// GENERIC Template Session Control OLC_START OLC_STOP OLC_SET
// --------------------------------------------

template <typename T>
void olc_send_value_error(CHAR_DATA* ch, const OlcField<T>& f, const std::string& value, int term_width, int indent)
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

template <typename T>
static bool olc_schema_has_field_name(const OlcSchema<T>* schema, const std::string& cmd)
{
    if (!schema || !schema->fields || cmd.empty())
        return false;

    if (olc_find_field_fuzzy(schema, cmd))
        return true;

    if (olc_field_name_is_ambiguous(schema, cmd))
        return true;

    return false;
}

template <typename T>
const OlcSchema<T>* olc_session_schema_as(const OlcSession* sess)
{
    if (!sess || !sess->schema)
        return nullptr;

    return static_cast<const OlcSchema<T>*>(sess->schema);
}

template <typename T>
T* olc_session_working_as(OlcSession* sess)
{
    if (!sess)
        return nullptr;

    return static_cast<T*>(sess->working_copy);
}

template <typename T>
const T* olc_session_working_as(const OlcSession* sess)
{
    if (!sess)
        return nullptr;

    return static_cast<const T*>(sess->working_copy);
}

template <typename T>
void olc_start(CHAR_DATA* ch, T* target, const OlcSchema<T>* schema, const OlcOps* ops)
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
    d->olc->schema = schema;   // stores as OlcSchemaBase*
    d->olc->ops = ops;
    d->olc->last_cmd_arg.clear();
    d->olc->dirty = false;
    d->olc->pending_exit_side_effects.clear();
    d->olc->mode = OlcEditMode::NONE;
    d->olc->anchor_room = nullptr;

    olc_mobile_reset_pending_proto(d->olc);
    if (schema->name && !str_cmp(schema->name, "mobile"))
        if constexpr (std::is_same_v<T, CHAR_DATA>)
        {
            olc_mobile_init_pending_proto_from_live(d->olc, target);
        }

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

template <typename T>
static bool olc_finish_editor_substate_typed(
    CHAR_DATA* ch,
    OlcSession* sess,
    const OlcSchema<T>* schema,
    T* obj)
{
    if (!ch || !sess || !schema || !schema->fields || !obj)
        return false;

    if (ch->substate <= SUB_NONE)
        return false;

    for (const auto& f : *schema->fields)
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

        bool ok = f.editor_setter(ch, obj, text);
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

template <typename T>
static void olc_set_typed(
    CHAR_DATA* ch,
    OlcSession* sess,
    T* obj,
    const OlcSchema<T>* schema,
    const std::string& field,
    const std::string& value)
{
    if (!ch || !sess || !obj || !schema || !schema->fields)
        return;

    int term_width = 80;
    int indent = 2;

    if (ch->desc && ch->desc->term_width > 0)
        term_width = ch->desc->term_width;

    if (field.empty())
    {
        send_to_char("Set what?\n", ch);
        return;
    }

    if (olc_field_name_is_ambiguous(schema, field))
    {
        auto matches = olc_ambiguous_field_matches(schema, field);
        send_to_char("Ambiguous field. Did you mean:\n", ch);
        send_to_char(olc_format_columns(matches, term_width, indent).c_str(), ch);
        return;
    }

    const auto* f = olc_find_field_fuzzy(schema, field);
    if (!f)
    {
        send_to_char("Unknown field.\n", ch);
        return;
    }

    if (f->value_type == OlcValueType::EDITOR)
    {
        if (!f->editor_takes_argument && !value.empty())
        {
            send_to_char("Use without value to enter editor.\n", ch);
            return;
        }

        sess->last_cmd_arg = value;

        if (!f->editor)
        {
            send_to_char("Field is not editable.\n", ch);
            return;
        }

        f->editor(ch, obj);
        return;
    }

    if (value.empty())
    {
        send_to_char("No value provided.\n", ch);
        return;
    }

    bool ok = false;

    if (f->setter)
        ok = f->setter(obj, value);
    else if (f->editor_setter)
        ok = f->editor_setter(ch, obj, value);
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

// OLC_SHOW templates - handlers for specific field types

template <typename T>
bool olc_show_handle_meta_list(
    const OlcField<T>& f,
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


template <typename T>
bool olc_show_handle_extradesc_list(
    CHAR_DATA* ch,
    T* working_copy,
    const OlcField<T>& f,
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

    std::string summary = olc_get_field_display_value_typed(ch, working_copy, f);

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

template <typename T>
bool olc_show_handle_exit_list(
    CHAR_DATA* ch,
    const OlcField<T>& f,
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

template <typename T>
static bool olc_show_handle_object_affect_list(
    CHAR_DATA* ch,
    T* working_copy,
    const OlcField<T>& f,
    OBJ_DATA* obj,
    const std::string& value,
    size_t max_name_len,
    int term_width,
    std::vector<std::string>& other_lines)
{
    if (f.meta_type != OlcMetaType::OBJ_AFFECT_LIST)
        return false;

    if (!obj)
        return false;

    if (!value.empty())
    {
        int index = atoi(value.c_str());
        if (index <= 0)
        {
            send_to_char("Affect number must be greater than 0.\n", ch);
            return true;
        }

        AFFECT_DATA* paf = olc_find_object_affect_by_number(obj, index);
        if (!paf)
        {
            send_to_char("No such affect.\n", ch);
            return true;
        }

        std::vector<std::string> lines =
            olc_object_format_single_affect_lines(paf, max_name_len, term_width);

        olc_append_lines(other_lines, lines);
        return true;
    }

    std::string summary = olc_get_field_display_value_typed(ch, working_copy, f);

    if (summary.empty())
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

template <typename T>
static bool olc_show_handle_mobile_affect_list(
    CHAR_DATA* ch,
    T* working_copy,
    const OlcField<T>& f,
    CHAR_DATA* mob,
    const std::string& value,
    size_t max_name_len,
    int term_width,
    std::vector<std::string>& other_lines)
{
    if (f.meta_type != OlcMetaType::MOB_AFFECT_LIST)
        return false;

    if (!mob)
        return false;

    if (!value.empty())
    {
        int index = atoi(value.c_str());
        if (index <= 0)
        {
            send_to_char("Affect number must be greater than 0.\n", ch);
            return true;
        }

        AFFECT_DATA* paf = olc_mobile_find_affect_by_number(mob, index);
        if (!paf)
        {
            send_to_char("No such affect.\n", ch);
            return true;
        }

        std::vector<std::string> lines =
            olc_mobile_format_single_affect_lines(paf, max_name_len, term_width);

        olc_append_lines(other_lines, lines);
        return true;
    }

    std::string summary = olc_get_field_display_value_typed(ch, working_copy, f);

    if (summary.empty())
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

template <typename T>
static bool olc_show_handle_special_list_field(
    CHAR_DATA* ch,
    T* working_copy,
    const OlcField<T>& f,
    ROOM_INDEX_DATA* room,
    OBJ_DATA* obj,
    CHAR_DATA* mob,
    const std::string& value,
    size_t max_name_len,
    int term_width,
    std::vector<std::string>& other_lines)
{
    if (olc_show_handle_extradesc_list(
            ch,
            working_copy,
            f,
            room,
            obj,
            value,
            max_name_len,
            term_width,
            other_lines))
        return true;

    if (olc_show_handle_exit_list(
            ch,
            f,
            room,
            max_name_len,
            term_width,
            other_lines))
        return true;

    if (olc_show_handle_object_affect_list(
            ch,
            working_copy,
            f,
            obj,
            value,
            max_name_len,
            term_width,
            other_lines))
        return true;

    if (olc_show_handle_mobile_affect_list(
            ch,
            working_copy,
            f,
            mob,
            value,
            max_name_len,
            term_width,
            other_lines))
        return true;

    return false;
}
// -------------------------------------------
// OLC_SHOW main templates
// -------------------------------------------

template <typename T>
static void olc_show_help_typed(
    CHAR_DATA* ch,
    const OlcSchema<T>* schema,
    const std::string& field)
{
    if (!ch || !schema || !schema->fields)
        return;

    int term_width = (ch->desc && ch->desc->term_width > 0) ? ch->desc->term_width : 80;
    int indent = 2;

    if (field.empty() || !str_cmp(field.c_str(), "help"))
    {
        std::vector<std::string> names = olc_all_field_names(schema);
        send_to_char("Valid fields:\n", ch);
        send_to_char(olc_format_columns(names, term_width, indent).c_str(), ch);
        return;
    }

    if (olc_field_name_is_ambiguous(schema, field))
    {
        auto matches = olc_ambiguous_field_matches(schema, field);
        send_to_char("Ambiguous field. Did you mean:\n", ch);
        send_to_char(olc_format_columns(matches, term_width, indent).c_str(), ch);
        return;
    }

    const auto* f = olc_find_field_fuzzy(schema, field);
    if (!f)
    {
        send_to_char("Unknown field.\n", ch);

        auto suggestions = olc_field_suggestions(schema, field);
        if (!suggestions.empty())
        {
            send_to_char("Did you mean:\n", ch);
            send_to_char(olc_format_columns(suggestions, term_width, indent).c_str(), ch);
        }
        return;
    }

    if (f->help && f->help[0] != '\0')
    {
        send_to_char(f->help, ch);
        send_to_char("\n", ch);
    }

    auto values = olc_enum_values_vec(*f);
    if (!values.empty())
    {
        send_to_char("Valid values:\n", ch);
        send_to_char(olc_format_columns(values, term_width, indent).c_str(), ch);
    }
}

template <typename T>
static void olc_show_typed(
    CHAR_DATA* ch,
    OlcSession* sess,
    T* obj,
    const OlcSchema<T>* schema,
    const std::string& field,
    const std::string& value,
    int term_width)
{
    if (!ch || !sess || !obj || !schema || !schema->fields)
    {
        send_to_char("You are not editing anything.\n", ch);
        return;
    }

    bool show_help = false;
    bool found = false;

    ROOM_INDEX_DATA* room = nullptr;
    OBJ_DATA* objp = nullptr;
    CHAR_DATA* mob = nullptr;

    if (sess->mode == OlcEditMode::ROOM_INLINE)
        room = static_cast<ROOM_INDEX_DATA*>(sess->working_copy);
    else if (sess->mode == OlcEditMode::OBJECT_INLINE)
        objp = static_cast<OBJ_DATA*>(sess->working_copy);
    else if (sess->mode == OlcEditMode::MOBILE_INLINE)
        mob = static_cast<CHAR_DATA*>(sess->working_copy);

    if (!field.empty() &&
        (!str_cmp(field.c_str(), "help") || !str_cmp(value.c_str(), "help")))
        show_help = true;

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

        if (objp)
        {
            olc_show_object(ch, objp, false, term_width);
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
     * Help mode is fully separate
     */
    if (show_help)
    {
        olc_show_help_typed(ch, schema, field);
        return;
    }

    /*
     * Generic catch-all field handling
     */
    std::vector<std::string> int_fields;
    std::vector<std::string> other_lines;

    size_t max_name_len = 0;

    for (const auto& f : *schema->fields)
    {
        if (!field.empty() && str_prefix_utf8(field.c_str(), f.name))
            continue;

        size_t len = strlen(f.name);
        if (len > max_name_len)
            max_name_len = len;
    }

    for (const auto& f : *schema->fields)
    {
        if (!field.empty() && str_prefix_utf8(field.c_str(), f.name))
            continue;

        found = true;

        if (olc_show_handle_special_list_field(
                ch,
                obj,
                f,
                room,
                objp,
                mob,
                value,
                max_name_len,
                term_width,
                other_lines))
        {
            continue;
        }

        std::string display_value = olc_get_field_display_value_typed(
            ch,
            obj,
            f);

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

        if (f.value_type == OlcValueType::INT)
        {
            std::string entry = std::string(f.name) + "=" + display_value;
            int_fields.push_back(entry);
            continue;
        }

        auto lines = olc_format_line(
            f.name,
            display_value,
            color,
            (int)max_name_len,
            term_width);

        for (const auto& l : lines)
            other_lines.push_back(l);
    }

    if (!found)
    {
        send_to_char("No matching field.\n", ch);
        return;
    }

    if (!int_fields.empty())
    {
        auto wrapped = olc_wrap_value_pairs(int_fields, term_width);
        for (const auto& line : wrapped)
        {
            send_to_char(line.c_str(), ch);
            send_to_char("\n", ch);
        }
    }

    for (const auto& line : other_lines)
    {
        send_to_char(line.c_str(), ch);
        send_to_char("\n", ch);
    }
}

const OlcSchema<ROOM_INDEX_DATA>* get_room_schema();
const OlcSchema<OBJ_DATA>* get_object_schema();
const OlcSchema<CHAR_DATA>* get_mobile_schema();

typedef AFFECT_DATA* (*olc_find_affect_by_number_fn)(void* obj, int index);
typedef bool (*olc_parse_affect_value_fn)(CHAR_DATA* ch, int loc, const std::string& value_text, int& out_value);
typedef AFFECT_DATA*& (*olc_affect_list_ref_fn)(void* obj);

// olc.c functions
void olc_stop(CHAR_DATA* ch, bool save);

void olc_set(CHAR_DATA* ch, const std::string& field, const std::string& value);
void olc_show(CHAR_DATA* ch, const std::string& field, const std::string& value);

CHAR_DATA* olc_mobile_clone(const CHAR_DATA* src);
void olc_mobile_free(CHAR_DATA* mob);
void olc_mobile_apply_instance_changes(CHAR_DATA* dst, const CHAR_DATA* src);

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
bool olc_object_edit_interpret(CHAR_DATA* ch, char* command, char* argument);

bool olc_room_delete_exit(ROOM_INDEX_DATA* room, EXIT_DATA* ex);
ROOM_INDEX_DATA* olc_room_clone(const ROOM_INDEX_DATA* src);
void olc_room_apply_changes(ROOM_INDEX_DATA* src, ROOM_INDEX_DATA* dst);
void olc_room_free(ROOM_INDEX_DATA* room);
void olc_show_room_preview(CHAR_DATA* ch, ROOM_INDEX_DATA* room, const char* argument);

bool olc_commit_current(CHAR_DATA* ch);
void olc_discard_current_working_copy(CHAR_DATA* ch);
AFFECT_DATA* olc_clone_affects(AFFECT_DATA* src);
void olc_free_affects(AFFECT_DATA* af);
bool olc_session_command_is_field_name(OlcSession* sess, const std::string& cmd);

bool olc_is_direction_alias(const char* cmd);
bool olc_command_matches(const char* input, const char* word);
std::vector<std::string> olc_format_line( const char* name, const std::string& value, const char* val_color, int max_name_len, int term_width);
std::vector<std::string> olc_format_exit_list_lines( ROOM_INDEX_DATA* room, size_t label_width, int term_width, bool show_help);
bool same_str(const char* a, const char* b);
bool olc_str_eq(const char* a, const char* b);
std::vector<std::string> olc_format_wrap_text( const std::string& text, int width, int indent);
bool olc_edit_affect_field_generic( CHAR_DATA* ch, void* target, olc_find_affect_by_number_fn find_affect_by_number, 
    olc_parse_affect_value_fn parse_affect_value, olc_affect_list_ref_fn first_affect_ref, olc_affect_list_ref_fn last_affect_ref);
std::vector<std::string> olc_format_pending_exit_side_effects( CHAR_DATA* ch, size_t label_width, int term_width);
void olc_replace_string(char*& dst, const char* src);
void olc_replace_string_idx(char*& dst, const char* src);
std::vector<std::string> olc_wrap_value_pairs( const std::vector<std::string>& pairs, int width);

