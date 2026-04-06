
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

struct OlcField
{
    const char* name;
    OlcValueType value_type;
    const void* meta = nullptr;
    OlcMetaType meta_type = OlcMetaType::NONE;

    std::function<bool(CHAR_DATA* ch, void* obj)> editor = nullptr;
    std::function<bool(void* obj, const std::string& value)> setter = nullptr;
    std::function<std::string(void* obj)> getter = nullptr;
    std::function<bool(CHAR_DATA* ch, void* obj, const std::string& value)> editor_setter = nullptr;    
    std::function<std::string(CHAR_DATA* ch, void* obj)> contextual_getter = nullptr;

    int editor_substate = 0;

    const char* help = nullptr;
    int min_int = INT_MIN;
    int max_int = INT_MAX;
    bool editor_takes_argument = false;
    
    OlcField(
        const char* n,
        OlcValueType value_type,
        const void* m,
        OlcMetaType mt,
        std::function<bool(CHAR_DATA* ch, void* obj)> e,
        std::function<bool(void*, const std::string&)> s,
        std::function<std::string(void*)> g,
        std::function<bool(CHAR_DATA* ch, void* obj, const std::string& value)> ess,
        std::function<std::string(CHAR_DATA* ch, void* obj)> cg,
        int es,
        const char* h = nullptr,
        int minv = INT_MIN,
        int maxv = INT_MAX,
        bool editor_arg = false
    )
        : name(n),
          value_type(value_type),
          meta(m),
          meta_type(mt),
          editor(std::move(e)),
          setter(std::move(s)),
          getter(std::move(g)),
          editor_setter(std::move(ess)),
          contextual_getter(std::move(cg)),
          editor_substate(es),
          help(h),
          min_int(minv),
          max_int(maxv),
          editor_takes_argument(editor_arg)
    {}    
};

template <typename T, typename M>
using olc_member_ptr = M T::*;

template <typename T, typename M>
OlcField make_olc_int_field(
    const char* name,
    olc_member_ptr<T, M> member,
    const char* help,
    int minv = INT_MIN,
    int maxv = INT_MAX)
{
    static_assert(std::is_integral<M>::value || std::is_enum<M>::value,
                  "make_olc_int_field requires an integral or enum member");

    return OlcField{
        name,
        OlcValueType::INT,
        nullptr,
        OlcMetaType::NONE,
        nullptr,
        [member, minv, maxv](void* obj, const std::string& value) -> bool
        {
            auto typed = static_cast<T*>(obj);
            return set_int_field(typed->*member, value, minv, maxv);
        },
        [member](void* obj) -> std::string
        {
            auto typed = static_cast<T*>(obj);
            return get_int_field(typed->*member);
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
OlcField make_olc_string_field(
    const char* name,
    olc_member_ptr<T, char*> member,
    const char* help)
{
    return OlcField{
        name,
        OlcValueType::STRING,
        nullptr,
        OlcMetaType::NONE,
        nullptr,
        [member](void* obj, const std::string& value) -> bool
        {
            auto typed = static_cast<T*>(obj);
            return set_str_field(typed->*member, value);
        },
        [member](void* obj) -> std::string
        {
            auto typed = static_cast<T*>(obj);
            return get_str_field(typed->*member);
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
OlcField make_olc_enum_legacy_field(
    const char* name,
    olc_member_ptr<T, M> member,
    const char* const* table,
    const char* help)
{
    static_assert(std::is_integral<M>::value || std::is_enum<M>::value,
                  "make_olc_enum_legacy_field requires an integral or enum member");

    return OlcField{
        name,
        OlcValueType::ENUM,
        (const void*)table,
        OlcMetaType::ENUM_LEGACY,
        nullptr,
        [member, table](void* obj, const std::string& value) -> bool
        {
            auto typed = static_cast<T*>(obj);

            int temp = 0;
            if (!set_enum_legacy_field(temp, value, table))
                return false;

            typed->*member = static_cast<M>(temp);
            return true;
        },
        [member, table](void* obj) -> std::string
        {
            auto typed = static_cast<T*>(obj);
            return get_enum_legacy_field(static_cast<int>(typed->*member), table);
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
OlcField make_olc_enum_flag_field(
    const char* name,
    olc_member_ptr<T, M> member,
    const flag_name* table,
    const char* help)
{
    static_assert(std::is_integral<M>::value || std::is_enum<M>::value,
                  "make_olc_enum_flag_field requires an integral or enum member");

    return OlcField{
        name,
        OlcValueType::ENUM,
        (const void*)table,
        OlcMetaType::ENUM_FLAG,
        nullptr,
        [member, table](void* obj, const std::string& value) -> bool
        {
            auto typed = static_cast<T*>(obj);

            int temp = 0;
            if (!set_enum_flag_field(temp, value, table))
                return false;

            typed->*member = static_cast<M>(temp);
            return true;
        },
        [member, table](void* obj) -> std::string
        {
            auto typed = static_cast<T*>(obj);
            return get_enum_flag_field(static_cast<int>(typed->*member), table);
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
OlcField make_olc_flag_field(
    const char* name,
    olc_member_ptr<T, M> member,
    const flag_name* table,
    const char* help)
{
    return OlcField{
        name,
        OlcValueType::FLAG,
        (const void*)table,
        OlcMetaType::FLAG_TABLE,
        nullptr,
        [member, table](void* obj, const std::string& value) -> bool
        {
            auto typed = static_cast<T*>(obj);
            return set_flag_field(typed->*member, value, table);
        },
        [member, table](void* obj) -> std::string
        {
            auto typed = static_cast<T*>(obj);
            return get_flag_field(typed->*member, table);
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
OlcField make_olc_editor_field(
    const char* name,
    olc_member_ptr<T, char*> member,
    int substate_val,
    const char* help)
{
    return OlcField{
        name,
        OlcValueType::EDITOR,
        nullptr,
        OlcMetaType::NONE,
        [member, substate_val](CHAR_DATA* ch, void* obj) -> bool
        {
            auto typed = static_cast<T*>(obj);
            ch->substate = substate_val;
            ch->dest_buf = typed;
            ch->last_cmd = do_olcset;
            start_editing(ch, typed->*member);
            return true;
        },
        nullptr,
        [member](void* obj) -> std::string
        {
            auto typed = static_cast<T*>(obj);
            return (typed->*member) ? (typed->*member) : "";
        },
        [member](CHAR_DATA* /*ch*/, void* obj, const std::string& value) -> bool
        {
            auto typed = static_cast<T*>(obj);
            return set_str_field(typed->*member, value);
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
OlcField make_olc_custom_editor_field(
    const char* name,
    OlcMetaType meta_type,
    std::function<bool(CHAR_DATA* ch, T* obj)> editor,
    std::function<std::string(T* obj)> getter,
    int editor_substate,
    const char* help,
    bool editor_takes_argument = true)
{
    return OlcField{
        name,
        OlcValueType::EDITOR,
        nullptr,
        meta_type,
        [editor](CHAR_DATA* ch, void* obj) -> bool
        {
            return editor(ch, static_cast<T*>(obj));
        },
        nullptr,
        [getter](void* obj) -> std::string
        {
            return getter(static_cast<T*>(obj));
        },
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
OlcField make_olc_bool_field(
    const char* name,
    olc_member_ptr<T, bool> member,
    const char* help)
{
    return OlcField{
        name,
        OlcValueType::BOOL,
        nullptr,
        OlcMetaType::NONE,
        nullptr,
        [member](void* obj, const std::string& value) -> bool
        {
            auto typed = static_cast<T*>(obj);
            return set_bool_field(typed->*member, value);
        },
        [member](void* obj) -> std::string
        {
            auto typed = static_cast<T*>(obj);
            return get_bool_field(typed->*member);
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
OlcField make_olc_legacy_flag_field(
    const char* name,
    olc_member_ptr<T, BitSet> member,
    const char* const* table,
    const char* help)
{
    return OlcField{
        name,
        OlcValueType::FLAG,
        (const void*)table,
        OlcMetaType::ENUM_LEGACY,
        nullptr,
        [member, table](void* obj, const std::string& value) -> bool
        {
            auto typed = static_cast<T*>(obj);
            return bitset_apply_from_legacy_string(typed->*member, value, table);
        },
        [member, table](void* obj) -> std::string
        {
            auto typed = static_cast<T*>(obj);
            return get_legacy_flag_field(typed->*member, table);
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

struct OlcSchema
{
    const char* name;
    const std::vector<OlcField>* fields;
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
    const OlcSchema* schema = nullptr;
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


typedef AFFECT_DATA* (*olc_find_affect_by_number_fn)(void* obj, int index);
typedef bool (*olc_parse_affect_value_fn)(CHAR_DATA* ch, int loc, const std::string& value_text, int& out_value);
typedef AFFECT_DATA*& (*olc_affect_list_ref_fn)(void* obj);

// olc.c functions
void olc_start(CHAR_DATA* ch, void* target, const OlcSchema* schema, const OlcOps* ops);
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

const OlcField* olc_find_field_fuzzy(const OlcSchema* schema, const std::string& input);
bool olc_field_name_is_ambiguous(const OlcSchema* schema, const std::string& input);
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

