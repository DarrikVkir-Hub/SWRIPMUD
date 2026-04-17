

#include <memory>
#include <map>

enum class CraftOutputMode
{
    CreateFromPrototype,
    TransformBaseMaterial
};

enum class CraftConsumeMode
{
    Keep,
    Consume,
    TransformBase
};

enum class CraftRoomRequirement
{
    None,
    Factory
};


struct CraftExpr
{
    std::string op;   // "const", "skill", "var", "add", "sub", "mul", "div", "min", "max"
    int constant = 0;
    std::string var_name;

    std::shared_ptr<CraftExpr> left;
    std::shared_ptr<CraftExpr> right;
    std::shared_ptr<CraftExpr> third;
    std::shared_ptr<CraftExpr> fourth;    
};

enum class CraftAffectTarget
{
    AffectValue,
    AffectDuration
};

struct CraftAffectDef
{
    int location = APPLY_NONE;          // APPLY_HITROLL, APPLY_DAMROLL, etc.
    CraftExpr modifier_expr;            // the modifier/value of the affect
    int bitvector = 0;                  // optional affect/weapon/resist flag if needed later
    bool use_duration = false;
    CraftExpr duration_expr;            // optional; usually 0 for object affects
};

struct CraftSlotDef
{
    std::string name;              // "fabric", "battery", "crystal"
    int item_type = -1;
    int min_count = 0;
    int max_count = 0;             // 0 means unlimited
    CraftConsumeMode consume_mode = CraftConsumeMode::Consume;
    bool may_be_base_material = false;
};

struct CraftDerivedVarDef
{
    std::string var_name;
    std::string slot_name;
    std::string source_field;   // "value0", "value1", "weight", "cost", "level"
    std::string aggregate_op;   // "first", "highest", "lowest", "sum", "count"

    bool use_clamp = false;
    int clamp_min = 0;

    bool clamp_max_from_skill_mult = false;
    int clamp_max_skill_mult = 0;

    bool clamp_max_constant = false;
    int clamp_max_value = 0;
};

struct CraftAssignment
{
    std::string target;  // "value0" ... "value5", "weight", "cost", "level"
    CraftExpr expr;
};

struct CraftRecipe
{
    int gsn = -1;
    std::string name;              // internal / command name
    std::string display_name;      // shown to user

    CraftRoomRequirement room_req = CraftRoomRequirement::None;
    int timer_ticks = 0;

    CraftOutputMode output_mode = CraftOutputMode::CreateFromPrototype;
    int result_vnum = 0;           // only used for CreateFromPrototype
    int result_item_type = -1;     // used in both modes

    bool allow_custom_name = true;
    bool requires_extra_arg = false;
    std::string extra_arg_name;    // "wearloc", etc.

    std::vector<int> set_wear_flags;
    std::vector<int> set_objflags;

    std::string name_template;         // "{name} bowcaster"
    std::string short_template;        // "{name}"
    std::string description_template;  // "{name} was dropped here."

    std::vector<CraftSlotDef> slots;
    std::vector<CraftDerivedVarDef> derived_vars;
    std::vector<CraftAssignment> assignments;

    std::vector<CraftAffectDef> affects;
    bool disabled = false;
    std::vector<int> blocked_wear_flags;
};

struct CraftSelectedItem
{
    std::string slot_name;
    OBJ_DATA *obj = nullptr;
};

struct CraftSession
{
    std::string recipe_name;
    std::string custom_name;
    std::string extra_arg;

    std::vector<CraftSelectedItem> selected;
    OBJ_DATA *base_material = nullptr;

    std::map<std::string, int> vars;
};

static inline CraftSession *craft_get_session( CHAR_DATA *ch )
{
    return ch ? ch->craft_session : nullptr;
}

inline void craft_clear_session( CHAR_DATA *ch )
{
    if ( !ch || !ch->craft_session )
        return;

    delete ch->craft_session;
    ch->craft_session = nullptr;
}

static inline CraftSession *craft_create_session( CHAR_DATA *ch )
{
    craft_clear_session( ch );
    ch->craft_session = new CraftSession();
    return ch->craft_session;
}

static inline void craft_enter_mode( CHAR_DATA *ch )
{
    if ( !ch )
        return;

    if ( !craft_get_session(ch) )
        craft_create_session( ch );

    ch->substate = SUB_CRAFT_MENU;
}

static inline void craft_exit_mode( CHAR_DATA *ch )
{
    if ( !ch )
        return;

    ch->substate = SUB_NONE;
    craft_clear_session( ch );
}

bool craft_parse_expr_for_olc( const std::string &text, CraftExpr &out, std::string &err );
std::string craft_expr_to_string_for_olc( const CraftExpr &expr );