

#include "mud.h"
#include "craft.h"
#include "olc.h"

extern int      top_affect;
extern const OlcItemValueInfo g_olc_item_value_info[];
extern const size_t g_olc_item_value_info_count;
extern char *fread_string_core(FILE *fp);
extern const OlcOps craft_olc_ops;
std::vector<CraftRecipe>& craft_get_recipes_for_olc();
bool craft_save_all_recipes_for_olc(CHAR_DATA* ch);


static const char *CRAFT_RECIPE_FILE = "../system/craft_recipes.dat";

static const CraftRecipe *craft_get_session_recipe( const CraftSession *sess );
static std::string craft_expr_to_string( const CraftExpr &expr );
static std::vector<CraftRecipe> &craft_get_recipes();
static bool craft_expr_from_string( const std::string &text, CraftExpr &out, std::string &err );

static void craft_show_help( CHAR_DATA *ch, const std::string &field )
{
    if ( field.empty() )
    {
        send_to_char(
            "&YCrafting Help&w\n"
            "  list                   - show learned recipes\n"
            "  select                 - show learned recipes\n"            
            "  select <recipe>        - choose a recipe\n"
            "  select auto            - auto-fill required slots from inventory\n"            
            "  show                   - show current crafting session\n"
            "  use <slot> <item>      - assign an item to a slot\n"
            "  unuse <slot> <item>    - remove an assigned item\n"
            "  base <item>            - choose base material\n"
            "  name <text>            - set crafted item name\n"
            "  set <field> <value>    - set recipe-specific field\n"
            "  build                  - begin crafting\n"
            "  pause                  - leave the crafting menu and keep your session\n"
            "  help [topic]           - show help\n"
            "  cancel                 - leave crafting mode\n",
            ch );
        return;
    }

    if ( !str_cmp( field, "use" ) )
    {
        send_to_char(
            "use <slot> <item>\n"
            "Assign an item from your inventory to one recipe slot.\n",
            ch );
        return;
    }

    if ( !str_cmp( field, "base" ) )
    {
        send_to_char(
            "base <item>\n"
            "Select the material that will be transformed into the final result\n"
            "for recipes that use a base material.\n",
            ch );
        return;
    }

    if ( !str_cmp( field, "set" ) )
    {
        send_to_char(
            "set <field> <value>\n"
            "Set a recipe-specific field such as wear location.\n",
            ch );
        return;
    }

    if ( !str_cmp( field, "build" ) )
    {
        send_to_char(
            "build\n"
            "Validate your selected materials and begin the timed crafting process.\n",
            ch );
        return;
    }

    send_to_char( "No craft help is available for that topic.\n", ch );
}

static CraftExpr craft_const( int v )
{
    CraftExpr e;
    e.op = "const";
    e.constant = v;
    return e;
}

static CraftExpr craft_skill()
{
    CraftExpr e;
    e.op = "skill";
    return e;
}

static int craft_skill_level( CHAR_DATA *ch, int gsn )
{
    if ( !ch )
        return 0;

    if ( IS_NPC(ch) )
        return ch->top_level;

    if ( gsn < 0 )
        return 0;

    return ch->pcdata->learned[gsn];
}

static bool craft_knows_recipe( CHAR_DATA *ch, const CraftRecipe *recipe )
{
    if ( !ch || !recipe )
        return false;

    if ( recipe->disabled )
        return false;

    if ( IS_NPC(ch) )
        return true;

    return recipe->gsn >= 0 && ch->pcdata->learned[recipe->gsn] > 0;
}

static bool craft_check_room( CHAR_DATA *ch, const CraftRecipe *recipe, std::string &err )
{
    if ( !ch || !recipe || !ch->in_room )
    {
        err = "You cannot craft here.";
        return false;
    }

    switch ( recipe->room_req )
    {
        default:
        case CraftRoomRequirement::None:
            return true;

        case CraftRoomRequirement::Factory:
            if ( !BV_IS_SET( ch->in_room->room_flags, ROOM_FACTORY ) )
            {
                err = "You need to be in a factory or workshop to do that.";
                return false;
            }
            return true;
    }
}

static const CraftSlotDef *craft_find_slot( const CraftRecipe *recipe, const std::string &slot_name )
{
    if ( !recipe )
        return nullptr;

    for ( const auto &slot : recipe->slots )
        if ( !str_cmp( slot_name, slot.name ) )
            return &slot;

    return nullptr;
}

static const OlcItemValueInfo *craft_get_item_value_info( int item_type )
{
    for ( size_t i = 0; i < g_olc_item_value_info_count; ++i )
    {
        if ( g_olc_item_value_info[i].item_type == item_type )
            return &g_olc_item_value_info[i];
    }
    return nullptr;
}

static const CraftSlotDef *craft_get_selected_slot_def( CraftSession *sess, const CraftSelectedItem &sel )
{
    const CraftRecipe *recipe = craft_get_session_recipe( sess );
    if ( !sess || !recipe )
        return nullptr;

    return craft_find_slot( recipe, sel.slot_name );
}

static int craft_target_to_value_index( const std::string &target )
{
    if ( !str_cmp( target, "value0" ) ) return 0;
    if ( !str_cmp( target, "value1" ) ) return 1;
    if ( !str_cmp( target, "value2" ) ) return 2;
    if ( !str_cmp( target, "value3" ) ) return 3;
    if ( !str_cmp( target, "value4" ) ) return 4;
    if ( !str_cmp( target, "value5" ) ) return 5;
    return -1;
}

static int craft_count_selected_object( CraftSession *sess, OBJ_DATA *obj )
{
    int count = 0;

    if ( !sess || !obj )
        return 0;

    for ( const auto &sel : sess->selected )
    {
        if ( sel.obj == obj )
            ++count;
    }

    return count;
}

static std::vector<OBJ_DATA*> craft_get_selected_for_slot( CraftSession *sess, const std::string &slot_name )
{
    std::vector<OBJ_DATA*> out;

    const CraftRecipe *recipe = craft_get_session_recipe( sess );
    if ( !sess || !recipe )
        return out;

    for ( const auto &sel : sess->selected )
        if ( !str_cmp( sel.slot_name, slot_name ) )
            out.push_back( sel.obj );

    return out;
}

static int craft_count_selected( CraftSession *sess, const CraftSlotDef *slot )
{
    int count = 0;

    if ( !sess || !slot )
        return 0;

    for ( const auto &sel : sess->selected )
        if ( !str_cmp( sel.slot_name, slot->name ) )
            ++count;

    return count;
}

static int craft_get_object_field( OBJ_DATA *obj, const std::string &field )
{
    if ( !obj )
        return 0;

    if ( !str_cmp( field, "value0" ) ) return obj->value[0];
    if ( !str_cmp( field, "value1" ) ) return obj->value[1];
    if ( !str_cmp( field, "value2" ) ) return obj->value[2];
    if ( !str_cmp( field, "value3" ) ) return obj->value[3];
    if ( !str_cmp( field, "value4" ) ) return obj->value[4];
    if ( !str_cmp( field, "value5" ) ) return obj->value[5];
    if ( !str_cmp( field, "weight" ) ) return obj->weight;
    if ( !str_cmp( field, "cost" ) )   return obj->cost;
    if ( !str_cmp( field, "level" ) )  return obj->level;

    return 0;
}

static int craft_compute_derived_var( CHAR_DATA *ch, CraftSession *sess, const CraftDerivedVarDef &def )
{
    const CraftRecipe *recipe = craft_get_session_recipe( sess );
    if ( !sess || !recipe )
        return 0;

    std::vector<OBJ_DATA*> items = craft_get_selected_for_slot( sess, def.slot_name );
    int result = 0;
    bool first = true;

    if ( !str_cmp( def.aggregate_op, "count" ) )
    {
        result = (int)items.size();
    }
    else
    {
        for ( OBJ_DATA *obj : items )
        {
            int val = craft_get_object_field( obj, def.source_field );

            if ( first )
            {
                result = val;
                first = false;

                if ( !str_cmp( def.aggregate_op, "first" ) )
                    break;

                continue;
            }

            if ( !str_cmp( def.aggregate_op, "highest" ) )
                result = UMAX( result, val );
            else if ( !str_cmp( def.aggregate_op, "lowest" ) )
                result = UMIN( result, val );
            else if ( !str_cmp( def.aggregate_op, "sum" ) )
                result += val;
        }
    }

    if ( def.use_clamp )
    {
        int maxv = result;

        if ( def.clamp_max_from_skill_mult )
        {
            int skill = craft_skill_level( ch, recipe->gsn );
            maxv = skill * def.clamp_max_skill_mult;
        }
        else if ( def.clamp_max_constant )
        {
            maxv = def.clamp_max_value;
        }

        result = URANGE( def.clamp_min, result, maxv );
    }

    return result;
}

static void craft_seed_builtin_vars( CraftSession *sess )
{
    if ( !sess )
        return;

    if ( sess->base_material )
    {
        sess->vars["base_value0"] = sess->base_material->value[0];
        sess->vars["base_value1"] = sess->base_material->value[1];
        sess->vars["base_value2"] = sess->base_material->value[2];
        sess->vars["base_value3"] = sess->base_material->value[3];
        sess->vars["base_value4"] = sess->base_material->value[4];
        sess->vars["base_value5"] = sess->base_material->value[5];
        sess->vars["base_weight"] = sess->base_material->weight;
        sess->vars["base_cost"]   = sess->base_material->cost;
        sess->vars["base_level"]  = sess->base_material->level;
    }
}

static bool craft_recompute_vars( CHAR_DATA *ch, CraftSession *sess, std::string &err )
{
    const CraftRecipe *recipe = craft_get_session_recipe( sess );
    if ( !sess || !recipe )
    {
        err = "No recipe selected.";
        return false;
    }

    sess->vars.clear();
    craft_seed_builtin_vars( sess );

    for ( const auto &def : recipe->derived_vars )
        sess->vars[def.var_name] = craft_compute_derived_var( ch, sess, def );

    return true;
}

static std::string craft_bool_str( bool v )
{
    return v ? "1" : "0";
}

static bool craft_bool_from_string( const std::string &text, bool &out )
{
    if ( !str_cmp( text, "1" ) || !str_cmp( text, "true" ) || !str_cmp( text, "yes" ) )
    {
        out = true;
        return true;
    }

    if ( !str_cmp( text, "0" ) || !str_cmp( text, "false" ) || !str_cmp( text, "no" ) )
    {
        out = false;
        return true;
    }

    return false;
}

static const char *craft_room_req_name( CraftRoomRequirement req )
{
    switch ( req )
    {
        default:
        case CraftRoomRequirement::None:    return "none";
        case CraftRoomRequirement::Factory: return "factory";
    }
}

static bool craft_room_req_from_name( const std::string &name, CraftRoomRequirement &out )
{
    if ( !str_cmp( name, "none" ) )
    {
        out = CraftRoomRequirement::None;
        return true;
    }

    if ( !str_cmp( name, "factory" ) )
    {
        out = CraftRoomRequirement::Factory;
        return true;
    }

    return false;
}

static const char *craft_output_mode_name( CraftOutputMode mode )
{
    switch ( mode )
    {
        default:
        case CraftOutputMode::CreateFromPrototype:   return "create_proto";
        case CraftOutputMode::TransformBaseMaterial: return "transform_base";
    }
}

static bool craft_output_mode_from_name( const std::string &name, CraftOutputMode &out )
{
    if ( !str_cmp( name, "create_proto" ) )
    {
        out = CraftOutputMode::CreateFromPrototype;
        return true;
    }

    if ( !str_cmp( name, "transform_base" ) )
    {
        out = CraftOutputMode::TransformBaseMaterial;
        return true;
    }

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

static bool craft_consume_mode_from_name( const std::string &name, CraftConsumeMode &out )
{
    if ( !str_cmp( name, "keep" ) )
    {
        out = CraftConsumeMode::Keep;
        return true;
    }

    if ( !str_cmp( name, "consume" ) )
    {
        out = CraftConsumeMode::Consume;
        return true;
    }

    if ( !str_cmp( name, "transform_base" ) )
    {
        out = CraftConsumeMode::TransformBase;
        return true;
    }

    return false;
}

static bool craft_recipe_blocks_wearloc( const CraftRecipe *recipe, int wearflag )
{
    if ( !recipe )
        return false;

    for ( int blocked : recipe->blocked_wear_flags )
    {
        if ( blocked == wearflag )
            return true;
    }

    return false;
}

static const std::string craft_item_type_name( int item_type )
{
    return get_flag_name(o_types, item_type, ITEMTYPE_MAX);
}

static int craft_item_type_from_name( const std::string &name )
{
    return get_otype( name );
}

static const char *craft_gsn_name( int gsn )
{
    if ( gsn < 0 || !skill_table[gsn] )
        return "none";

    return skill_table[gsn]->name;
}

static int craft_gsn_from_name( const std::string &name )
{
    return skill_lookup(name);
}

static std::string fread_std_string( FILE *fp )
{
    return std::string( fread_string_core( fp ) );
}

static void save_one_craft_recipe( FILE *fp, const CraftRecipe &recipe )
{
    fprintf( fp, "#CRAFT\n" );
    fprintf( fp, "Name %s~\n", recipe.name.c_str() );
    fprintf( fp, "DisplayName %s~\n", recipe.display_name.c_str() );
    fprintf( fp, "Gsn %s~\n", craft_gsn_name( recipe.gsn ) );
    fprintf( fp, "RoomReq %s~\n", craft_room_req_name( recipe.room_req ) );
    fprintf( fp, "Timer %d\n", recipe.timer_ticks );
    fprintf( fp, "OutputMode %s~\n", craft_output_mode_name( recipe.output_mode ) );
    fprintf( fp, "ResultVnum %d\n", recipe.result_vnum );
    fprintf( fp, "ResultItemType %s~\n", craft_item_type_name( recipe.result_item_type ).c_str() );
    fprintf( fp, "AllowCustomName %s~\n", craft_bool_str( recipe.allow_custom_name ).c_str() );
    fprintf( fp, "RequiresExtraArg %s~\n", craft_bool_str( recipe.requires_extra_arg ).c_str() );
    fprintf( fp, "ExtraArgName %s~\n", recipe.extra_arg_name.c_str() );
    fprintf( fp, "NameTemplate %s~\n", recipe.name_template.c_str() );
    fprintf( fp, "ShortTemplate %s~\n", recipe.short_template.c_str() );
    fprintf( fp, "DescriptionTemplate %s~\n", recipe.description_template.c_str() );

    for ( int flag : recipe.set_wear_flags )
        fprintf( fp, "WearFlag %d\n", flag );

    for ( int flag : recipe.set_objflags )
        fprintf( fp, "ObjFlag %d\n", flag );

    for ( int flag : recipe.blocked_wear_flags )
        fprintf( fp, "BlockedWearFlag %d\n", flag );

    for ( const auto &slot : recipe.slots )
    {
        fprintf( fp, "Slot %s~ %s~ %d %d %s~ %s~ %s~\n",
            slot.name.c_str(),
            craft_item_type_name( slot.item_type ).c_str(),
            slot.min_count,
            slot.max_count,
            "true", // optional is no longer supported, but write true for backward compatibility
            craft_consume_mode_name( slot.consume_mode ),
            craft_bool_str( slot.may_be_base_material ).c_str() );
    }

    for ( const auto &dv : recipe.derived_vars )
    {
        fprintf( fp, "DerivedVar %s~ %s~ %s~ %s~ %s~ %d %s~ %d %s~ %d\n",
            dv.var_name.c_str(),
            dv.slot_name.c_str(),
            dv.source_field.c_str(),
            dv.aggregate_op.c_str(),
            craft_bool_str( dv.use_clamp ).c_str(),
            dv.clamp_min,
            craft_bool_str( dv.clamp_max_from_skill_mult ).c_str(),
            dv.clamp_max_skill_mult,
            craft_bool_str( dv.clamp_max_constant ).c_str(),
            dv.clamp_max_value );
    }

    for ( const auto &asgn : recipe.assignments )
    {
        std::string expr_text = craft_expr_to_string( asgn.expr );
        fprintf( fp, "Assign %s~ %s~\n",
            asgn.target.c_str(),
            expr_text.c_str() );
    }

    for ( const auto &aff : recipe.affects )
    {
        std::string mod_expr = craft_expr_to_string( aff.modifier_expr );
        std::string dur_expr = craft_expr_to_string( aff.duration_expr );

        fprintf( fp, "Affect %d %s~ %d %s~ %s~\n",
            aff.location,
            mod_expr.c_str(),
            aff.bitvector,
            craft_bool_str( aff.use_duration ).c_str(),
            dur_expr.c_str() );
    }

    fprintf( fp, "End\n\n" );
}

static void save_all_craft_recipes()
{
    FILE *fp = fopen( CRAFT_RECIPE_FILE, "w" );

    if ( !fp )
    {
        bug( "%s: cannot open %s for writing", __func__, CRAFT_RECIPE_FILE );
        return;
    }

    for ( const auto &recipe : craft_get_recipes() )
        save_one_craft_recipe( fp, recipe );

    fprintf( fp, "#END\n" );
    fclose( fp );
}

static bool load_one_craft_recipe( FILE *fp, CraftRecipe &recipe )
{
    for ( ;; )
    {
        const char *word = fread_word( fp );

        if ( !str_cmp( word, "End" ) )
            return true;

        if ( !str_cmp( word, "Name" ) )
            recipe.name = fread_std_string( fp );
        else if ( !str_cmp( word, "DisplayName" ) )
            recipe.display_name = fread_std_string( fp );
        else if ( !str_cmp( word, "Gsn" ) )
        {
            recipe.gsn = craft_gsn_from_name( fread_std_string( fp ) );
            if ( recipe.gsn < 0 )
                recipe.gsn = -1;
        }
        else if ( !str_cmp( word, "RoomReq" ) )
        {
            std::string text = fread_std_string( fp );
            if ( !craft_room_req_from_name( text, recipe.room_req ) )
                recipe.room_req = CraftRoomRequirement::None;
        }
        else if ( !str_cmp( word, "Timer" ) )
            recipe.timer_ticks = fread_number( fp );
        else if ( !str_cmp( word, "OutputMode" ) )
        {
            std::string text = fread_std_string( fp );
            if ( !craft_output_mode_from_name( text, recipe.output_mode ) )
                recipe.output_mode = CraftOutputMode::CreateFromPrototype;
        }
        else if ( !str_cmp( word, "ResultVnum" ) )
            recipe.result_vnum = fread_number( fp );
        else if ( !str_cmp( word, "ResultItemType" ) )
        {
            int type = craft_item_type_from_name( fread_std_string( fp ) );
            if ( type < 0 )
            {
                bug( "%s: invalid item type in recipe '%s'", __func__, recipe.name.c_str() );
                type = ITEM_TRASH; // or ITEM_NONE if you have it
            }
            recipe.result_item_type = type;
        }
        else if ( !str_cmp( word, "AllowCustomName" ) )
        {
            bool b = false;
            if ( craft_bool_from_string( fread_std_string( fp ), b ) )
                recipe.allow_custom_name = b;
        }
        else if ( !str_cmp( word, "RequiresExtraArg" ) )
        {
            bool b = false;
            if ( craft_bool_from_string( fread_std_string( fp ), b ) )
                recipe.requires_extra_arg = b;
        }
        else if ( !str_cmp( word, "ExtraArgName" ) )
            recipe.extra_arg_name = fread_std_string( fp );
        else if ( !str_cmp( word, "NameTemplate" ) )
            recipe.name_template = fread_std_string( fp );
        else if ( !str_cmp( word, "ShortTemplate" ) )
            recipe.short_template = fread_std_string( fp );
        else if ( !str_cmp( word, "DescriptionTemplate" ) )
            recipe.description_template = fread_std_string( fp );
        else if ( !str_cmp( word, "WearFlag" ) )
            recipe.set_wear_flags.push_back( fread_number( fp ) );
        else if ( !str_cmp( word, "ObjFlag" ) )
            recipe.set_objflags.push_back( fread_number( fp ) );
        else if ( !str_cmp( word, "BlockedWearFlag" ) )
            recipe.blocked_wear_flags.push_back( fread_number( fp ) );            
        else if ( !str_cmp( word, "Slot" ) )
        {
            CraftSlotDef slot;
            bool b = false;
            std::string item_type_name;
            std::string optional_text;
            std::string consume_text;
            std::string base_text;

            slot.name = fread_std_string( fp );
            item_type_name = fread_std_string( fp );

            int type = craft_item_type_from_name( item_type_name );
            if ( type < 0 )
            {
                bug( "%s: invalid slot item type '%s' in recipe '%s'",
                    __func__, item_type_name.c_str(), recipe.name.c_str() );
                type = ITEM_TRASH;
            }
            slot.item_type = type;          
            slot.min_count = fread_number( fp );
            slot.max_count = fread_number( fp );
            optional_text = fread_std_string( fp ); // optional is no longer supported, but read it for backward compatibility
            consume_text = fread_std_string( fp );
            base_text = fread_std_string( fp );
            
            if ( craft_consume_mode_from_name( consume_text, slot.consume_mode ) == false )
                slot.consume_mode = CraftConsumeMode::Consume;
            if ( craft_bool_from_string( base_text, b ) )
                slot.may_be_base_material = b;

            recipe.slots.push_back( slot );
        }
        else if ( !str_cmp( word, "DerivedVar" ) )
        {
            CraftDerivedVarDef dv;
            bool b = false;
            std::string temp;

            dv.var_name = fread_std_string( fp );
            dv.slot_name = fread_std_string( fp );
            dv.source_field = fread_std_string( fp );
            dv.aggregate_op = fread_std_string( fp );

            temp = fread_std_string( fp );
            if ( craft_bool_from_string( temp, b ) )
                dv.use_clamp = b;

            dv.clamp_min = fread_number( fp );

            temp = fread_std_string( fp );
            if ( craft_bool_from_string( temp, b ) )
                dv.clamp_max_from_skill_mult = b;

            dv.clamp_max_skill_mult = fread_number( fp );

            temp = fread_std_string( fp );
            if ( craft_bool_from_string( temp, b ) )
                dv.clamp_max_constant = b;

            dv.clamp_max_value = fread_number( fp );

            recipe.derived_vars.push_back( dv );
        }
        else if ( !str_cmp( word, "Assign" ) )
        {
            CraftAssignment asgn;
            std::string expr_text;
            std::string err;

            asgn.target = fread_std_string( fp );
            expr_text = fread_std_string( fp );

            if ( !craft_expr_from_string( expr_text, asgn.expr, err ) )
            {
                bug( "%s: bad assignment expr '%s': %s", __func__, expr_text.c_str(), err.c_str() );
                asgn.expr = craft_const( 0 );
            }

            recipe.assignments.push_back( asgn );
        }
        else if ( !str_cmp( word, "Affect" ) )
        {
            CraftAffectDef aff;
            std::string expr_text;
            std::string temp;
            std::string err;

            aff.location = fread_number( fp );
            expr_text = fread_std_string( fp );
            aff.bitvector = fread_number( fp );
            temp = fread_std_string( fp );

            {
                bool b = false;
                if ( craft_bool_from_string( temp, b ) )
                    aff.use_duration = b;
            }

            if ( !craft_expr_from_string( expr_text, aff.modifier_expr, err ) )
            {
                bug( "%s: bad affect modifier expr '%s': %s", __func__, expr_text.c_str(), err.c_str() );
                aff.modifier_expr = craft_const( 0 );
            }

            expr_text = fread_std_string( fp );
            if ( !craft_expr_from_string( expr_text, aff.duration_expr, err ) )
                aff.duration_expr = craft_const( 0 );

            recipe.affects.push_back( aff );
        }
        else
        {
            bug( "%s: unknown word '%s' in %s", __func__, word, CRAFT_RECIPE_FILE );
        }
    }
}

static bool load_all_craft_recipes( std::vector<CraftRecipe> &out )
{
    FILE *fp = fopen( CRAFT_RECIPE_FILE, "r" );
    std::vector<CraftRecipe> loaded;

    if ( !fp )
        return false;

    for ( ;; )
    {
        const char *word = fread_word( fp );

        if ( !str_cmp( word, "#END" ) )
            break;

        if ( str_cmp( word, "#CRAFT" ) )
        {
            bug( "%s: expected #CRAFT, found '%s'", __func__, word );
            fclose( fp );
            return false;
        }

        CraftRecipe recipe;
        if ( !load_one_craft_recipe( fp, recipe ) )
        {
            fclose( fp );
            return false;
        }

        loaded.push_back( std::move(recipe) );
    }

    fclose( fp );

    if ( loaded.empty() )
        return false;

    out = std::move( loaded );
    return true;
}

std::vector<CraftRecipe> &craft_get_recipes_for_olc()
{
    return craft_get_recipes();
}

CraftRecipe *craft_find_recipe_for_olc( const std::string &name )
{
    if ( name.empty() )
        return nullptr;

    for ( auto &recipe : craft_get_recipes() )
    {
        if ( !str_cmp( name, recipe.name ) )
            return &recipe;
    }

    return nullptr;
}

void craft_save_all_recipes_for_olc()
{
    save_all_craft_recipes();
}

static int craft_eval_expr( CHAR_DATA *ch, CraftSession *sess, const CraftExpr &expr )
{
    const CraftRecipe *recipe = craft_get_session_recipe( sess );

    if ( !str_cmp( expr.op, "const" ) )
        return expr.constant;

    if ( !str_cmp( expr.op, "skill" ) )
        return recipe ? craft_skill_level( ch, recipe->gsn ) : 0;

    if ( !str_cmp( expr.op, "var" ) )
    {
        if ( !sess )
            return 0;

        auto it = sess->vars.find( expr.var_name );
        return ( it != sess->vars.end() ) ? it->second : 0;
    }

    if ( !expr.left || !expr.right )
        return 0;

    if ( !str_cmp( expr.op, "add" ) )
        return craft_eval_expr( ch, sess, *expr.left ) + craft_eval_expr( ch, sess, *expr.right );

    if ( !str_cmp( expr.op, "sub" ) )
        return craft_eval_expr( ch, sess, *expr.left ) - craft_eval_expr( ch, sess, *expr.right );

    if ( !str_cmp( expr.op, "mul" ) )
        return craft_eval_expr( ch, sess, *expr.left ) * craft_eval_expr( ch, sess, *expr.right );

    if ( !str_cmp( expr.op, "div" ) )
    {
        int rhs = craft_eval_expr( ch, sess, *expr.right );
        if ( rhs == 0 )
            return 0;
        return craft_eval_expr( ch, sess, *expr.left ) / rhs;
    }

    if ( !str_cmp( expr.op, "min" ) )
    {
        int lhs = craft_eval_expr( ch, sess, *expr.left );
        int rhs = craft_eval_expr( ch, sess, *expr.right );
        return UMIN( lhs, rhs );
    }

    if ( !str_cmp( expr.op, "max" ) )
    {
        int lhs = craft_eval_expr( ch, sess, *expr.left );
        int rhs = craft_eval_expr( ch, sess, *expr.right );
        return UMAX( lhs, rhs );
    }

    if ( !str_cmp( expr.op, "iflt" ) )
    {
        if ( !expr.left || !expr.right || !expr.third || !expr.fourth )
            return 0;

        int lhs = craft_eval_expr( ch, sess, *expr.left );
        int rhs = craft_eval_expr( ch, sess, *expr.right );

        if ( lhs < rhs )
            return craft_eval_expr( ch, sess, *expr.third );

        return craft_eval_expr( ch, sess, *expr.fourth );
    }

    return 0;
}

static std::string craft_trim_copy( const std::string &s )
{
    size_t start = 0;
    size_t end = s.size();

    while ( start < end && ( s[start] == ' ' || s[start] == '\t' || s[start] == '\r' || s[start] == '\n' ) )
        ++start;

    while ( end > start && ( s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r' || s[end - 1] == '\n' ) )
        --end;

    return s.substr( start, end - start );
}

static bool craft_is_valid_expr_op( const std::string &op )
{
    return !str_cmp( op, "add" )
        || !str_cmp( op, "sub" )
        || !str_cmp( op, "mul" )
        || !str_cmp( op, "div" )
        || !str_cmp( op, "min" )
        || !str_cmp( op, "max" );
}

static bool craft_parse_sexrace( const std::string &input, int &sex, int &race )
{
    std::istringstream iss( input );

    if ( !( iss >> sex >> race ) )
        return false;

    if ( sex < 0 || sex > 2 )
        return false;

    if ( race < 0 || race > 33 )
        return false;

    return true;
}

static bool craft_parse_int_str( const std::string &text, int &out )
{
    char *endptr = nullptr;
    long v;
    std::string trimmed = craft_trim_copy( text );

    if ( trimmed.empty() )
        return false;

    v = strtol( trimmed.c_str(), &endptr, 10 );
    if ( !endptr || *endptr != '\0' )
        return false;

    out = (int)v;
    return true;
}

static bool craft_split_expr_args4(
    const std::string &text,
    std::string &a,
    std::string &b,
    std::string &c,
    std::string &d )
{
    int depth = 0;
    size_t p1 = std::string::npos;
    size_t p2 = std::string::npos;
    size_t p3 = std::string::npos;

    for ( size_t i = 0; i < text.size(); ++i )
    {
        char ch = text[i];

        if ( ch == '(' )
            ++depth;
        else if ( ch == ')' )
        {
            if ( depth <= 0 )
                return false;
            --depth;
        }
        else if ( ch == ',' && depth == 0 )
        {
            if ( p1 == std::string::npos )
                p1 = i;
            else if ( p2 == std::string::npos )
                p2 = i;
            else if ( p3 == std::string::npos )
                p3 = i;
            else
                return false;
        }
    }

    if ( p1 == std::string::npos || p2 == std::string::npos || p3 == std::string::npos )
        return false;

    a = craft_trim_copy( text.substr( 0, p1 ) );
    b = craft_trim_copy( text.substr( p1 + 1, p2 - p1 - 1 ) );
    c = craft_trim_copy( text.substr( p2 + 1, p3 - p2 - 1 ) );
    d = craft_trim_copy( text.substr( p3 + 1 ) );

    return !a.empty() && !b.empty() && !c.empty() && !d.empty();
}

static bool craft_split_expr_args( const std::string &text, std::string &left, std::string &right )
{
    int depth = 0;

    for ( size_t i = 0; i < text.size(); ++i )
    {
        char c = text[i];

        if ( c == '(' )
            ++depth;
        else if ( c == ')' )
        {
            if ( depth <= 0 )
                return false;
            --depth;
        }
        else if ( c == ',' && depth == 0 )
        {
            left = craft_trim_copy( text.substr( 0, i ) );
            right = craft_trim_copy( text.substr( i + 1 ) );
            return !left.empty() && !right.empty();
        }
    }

    return false;
}

static std::string craft_expr_to_string( const CraftExpr &expr )
{
    if ( !str_cmp( expr.op, "const" ) )
        return str_printf( "const(%d)", expr.constant );

    if ( !str_cmp( expr.op, "skill" ) )
        return "skill";

    if ( !str_cmp( expr.op, "var" ) )
        return str_printf( "var(%s)", expr.var_name.c_str() );

    if ( !str_cmp( expr.op, "iflt" ) && expr.left && expr.right && expr.third && expr.fourth )
    {
        return str_printf( "iflt(%s,%s,%s,%s)",
            craft_expr_to_string( *expr.left ).c_str(),
            craft_expr_to_string( *expr.right ).c_str(),
            craft_expr_to_string( *expr.third ).c_str(),
            craft_expr_to_string( *expr.fourth ).c_str() );
    }        

    if ( craft_is_valid_expr_op( expr.op ) && expr.left && expr.right )
    {
        return str_printf( "%s(%s,%s)",
            expr.op.c_str(),
            craft_expr_to_string( *expr.left ).c_str(),
            craft_expr_to_string( *expr.right ).c_str() );
    }

    return "const(0)";
}

static bool craft_expr_from_string( const std::string &text, CraftExpr &out, std::string &err )
{
    std::string s = craft_trim_copy( text );

    out = CraftExpr();

    if ( s.empty() )
    {
        err = "Empty expression.";
        return false;
    }

    if ( !str_cmp( s, "skill" ) )
    {
        out.op = "skill";
        return true;
    }

    if ( s.size() > 7 && !str_prefix( "const(", s ) && s[s.size() - 1] == ')' )
    {
        std::string inner = s.substr( 6, s.size() - 7 );
        int value = 0;

        if ( !craft_parse_int_str( inner, value ) )
        {
            err = str_printf( "Invalid const() value: %s", inner.c_str() );
            return false;
        }

        out.op = "const";
        out.constant = value;
        return true;
    }

    if ( s.size() > 5 && !str_prefix( "var(", s ) && s[s.size() - 1] == ')' )
    {
        std::string inner = craft_trim_copy( s.substr( 4, s.size() - 5 ) );

        if ( inner.empty() )
        {
            err = "var() requires a variable name.";
            return false;
        }

        out.op = "var";
        out.var_name = inner;
        return true;
    }

    if ( s.size() > 6 && !str_prefix( "iflt(", s ) && s[s.size() - 1] == ')' )
    {
        std::string inner = s.substr( 5, s.size() - 6 );
        std::string a, b, c, d;
        CraftExpr ea, eb, ec, ed;

        if ( !craft_split_expr_args4( inner, a, b, c, d ) )
        {
            err = "Could not split arguments for iflt().";
            return false;
        }

        if ( !craft_expr_from_string( a, ea, err ) )
            return false;
        if ( !craft_expr_from_string( b, eb, err ) )
            return false;
        if ( !craft_expr_from_string( c, ec, err ) )
            return false;
        if ( !craft_expr_from_string( d, ed, err ) )
            return false;

        out.op = "iflt";
        out.left   = std::make_shared<CraftExpr>( std::move( ea ) );
        out.right  = std::make_shared<CraftExpr>( std::move( eb ) );
        out.third  = std::make_shared<CraftExpr>( std::move( ec ) );
        out.fourth = std::make_shared<CraftExpr>( std::move( ed ) );
        return true;
    }

    {
        size_t open = s.find( '(' );

        if ( open == std::string::npos || s[s.size() - 1] != ')' )
        {
            err = str_printf( "Malformed expression: %s", s.c_str() );
            return false;
        }

        std::string op = craft_trim_copy( s.substr( 0, open ) );
        std::string inner = s.substr( open + 1, s.size() - open - 2 );
        std::string left_text;
        std::string right_text;
        CraftExpr left_expr;
        CraftExpr right_expr;

        if ( !craft_is_valid_expr_op( op ) )
        {
            err = str_printf( "Unknown expression op: %s", op.c_str() );
            return false;
        }

        if ( !craft_split_expr_args( inner, left_text, right_text ) )
        {
            err = str_printf( "Could not split arguments for %s()", op.c_str() );
            return false;
        }

        if ( !craft_expr_from_string( left_text, left_expr, err ) )
            return false;

        if ( !craft_expr_from_string( right_text, right_expr, err ) )
            return false;

        out.op = op;
        out.left = std::make_shared<CraftExpr>( std::move(left_expr) );
        out.right = std::make_shared<CraftExpr>( std::move(right_expr) );
        return true;
    }
}

static bool craft_expr_roundtrip_ok( const CraftExpr &expr )
{
    CraftExpr parsed;
    std::string err;
    std::string text = craft_expr_to_string( expr );

    if ( !craft_expr_from_string( text, parsed, err ) )
        return false;

    return craft_expr_to_string( parsed ) == text;
}

static void craft_debug_expr_test( CHAR_DATA *ch, const std::string &text )
{
    CraftExpr expr;
    std::string err;

    if ( !craft_expr_from_string( text, expr, err ) )
    {
        ch_printf( ch, "Parse failed: %s\n", err.c_str() );
        return;
    }

    ch_printf( ch, "Parsed ok: %s\n", craft_expr_to_string( expr ).c_str() );

    if ( !craft_expr_roundtrip_ok( expr ) )
        send_to_char( "WARNING: roundtrip mismatch.\n", ch );
}

static bool craft_selected_items_still_valid( CHAR_DATA *ch, CraftSession *sess, std::string &err )
{
    if ( !ch || !sess )
        return false;

    const CraftRecipe *recipe = craft_get_session_recipe( sess );
    if ( !recipe )
    {
        err = "Your crafting recipe is no longer available.";
        return false;
    }

    for ( const auto &sel : sess->selected )
    {
        bool found = false;
        OBJ_DATA *obj;

        const CraftSlotDef *slot = craft_find_slot( recipe, sel.slot_name );
        if ( !slot )
        {
            err = "One of your selected slots no longer exists for this recipe.";
            return false;
        }

        for ( obj = ch->last_carrying; obj; obj = obj->prev_content )
        {
            if ( obj == sel.obj )
            {
                found = true;
                break;
            }
        }

        if ( !found )
        {
            err = "One of your selected materials is no longer in your inventory.";
            return false;
        }
    }

    if ( sess->base_material )
    {
        bool found = false;
        OBJ_DATA *obj;

        for ( obj = ch->last_carrying; obj; obj = obj->prev_content )
        {
            if ( obj == sess->base_material )
            {
                found = true;
                break;
            }
        }

        if ( !found )
        {
            err = "Your base material is no longer in your inventory.";
            return false;
        }
    }

    return true;
}

static CraftExpr craft_var( const std::string &name )
{
    CraftExpr e;
    e.op = "var";
    e.var_name = name;
    return e;
}

static CraftExpr craft_binop( const std::string &op, CraftExpr left, CraftExpr right )
{
    CraftExpr e;
    e.op = op;
    e.left = std::make_shared<CraftExpr>( std::move(left) );
    e.right = std::make_shared<CraftExpr>( std::move(right) );
    return e;
}

static bool craft_apply_one_affect( CHAR_DATA *ch, CraftSession *sess, OBJ_DATA *result,
                                    const CraftAffectDef &def, std::string &err )
{
    AFFECT_DATA *paf;
    int mod;

    const CraftRecipe *recipe = craft_get_session_recipe( sess );
    if ( !sess || !recipe || !result )
    {
        err = "Cannot apply recipe affect.";
        return false;
    }

    mod = craft_eval_expr( ch, sess, def.modifier_expr );

    CREATE( paf, AFFECT_DATA, 1 );
    paf->type = -1;
    paf->duration = def.use_duration ? craft_eval_expr( ch, sess, def.duration_expr ) : -1;
    paf->location = def.location;
    paf->modifier = mod;
    paf->bitvector = def.bitvector;

    LINK( paf, result->first_affect, result->last_affect, next, prev );
    ++top_affect;    
    return true;
}

static bool craft_apply_affects( CHAR_DATA *ch, CraftSession *sess, OBJ_DATA *result, std::string &err )
{
    const CraftRecipe *recipe = craft_get_session_recipe( sess );
    if ( !sess || !recipe || !result )    
    {
        err = "Cannot apply recipe affects.";
        return false;
    }

    for ( const auto &aff : recipe->affects )
    {
        if ( !craft_apply_one_affect( ch, sess, result, aff, err ) )
            return false;
    }

    return true;
}

static void craft_apply_extra_wearloc( CraftSession *sess, OBJ_DATA *result )
{
    int value;

    const CraftRecipe *recipe = craft_get_session_recipe( sess );
    if ( !sess || !recipe || !result )
        return;

    if ( !recipe->requires_extra_arg || str_cmp( recipe->extra_arg_name, "wearloc" ) )
        return;

    value = get_wflag( sess->extra_arg );
    if ( value < 0 )
        BV_SET_BIT( result->wear_flags, ITEM_WEAR_BODY );
    else
        BV_SET_BIT( result->wear_flags, value );
}

static void craft_apply_assignment( CHAR_DATA *ch, CraftSession *sess, OBJ_DATA *result, const CraftAssignment &asgn )
{
    int val = craft_eval_expr( ch, sess, asgn.expr );

    if ( !str_cmp( asgn.target, "value0" ) ) result->value[0] = val;
    else if ( !str_cmp( asgn.target, "value1" ) ) result->value[1] = val;
    else if ( !str_cmp( asgn.target, "value2" ) ) result->value[2] = val;
    else if ( !str_cmp( asgn.target, "value3" ) ) result->value[3] = val;
    else if ( !str_cmp( asgn.target, "value4" ) ) result->value[4] = val;
    else if ( !str_cmp( asgn.target, "value5" ) ) result->value[5] = val;
    else if ( !str_cmp( asgn.target, "weight" ) ) result->weight = val;
    else if ( !str_cmp( asgn.target, "cost" ) ) result->cost = val;
    else if ( !str_cmp( asgn.target, "level" ) ) result->level = val;
}

static bool craft_apply_assignments( CHAR_DATA *ch, CraftSession *sess, OBJ_DATA *result, std::string &err )
{
    const CraftRecipe *recipe = craft_get_session_recipe( sess );
    if ( !sess || !recipe || !result )    
    {
        err = "Cannot apply recipe assignments.";
        return false;
    }

    for ( const auto &asgn : recipe->assignments )
        craft_apply_assignment( ch, sess, result, asgn );

    return true;
}

struct CraftTemplateContext
{
    std::string custom_name;
    std::string extra_arg;
    std::string base_name;
    std::string base_short;
};

static void craft_replace_all( std::string &text, const std::string &token, const std::string &value )
{
    size_t pos = text.find( token );

    while ( pos != std::string::npos )
    {
        text.replace( pos, token.length(), value );
        pos = text.find( token, pos + value.length() );
    }
}

static std::string craft_expand_template(
    const std::string &templ,
    const CraftTemplateContext &ctx )
{
    std::string out = templ;

    craft_replace_all( out, "{name}", ctx.custom_name );
    craft_replace_all( out, "{extra}", ctx.extra_arg );
    craft_replace_all( out, "{base_name}", ctx.base_name );
    craft_replace_all( out, "{base_short}", ctx.base_short );

    return out;
}

static bool craft_create_result_object( CHAR_DATA *ch, CraftSession *sess, OBJ_DATA *&result, std::string &err )
{
    OBJ_INDEX_DATA *pObjIndex;
    CraftTemplateContext tmpl;

    const CraftRecipe *recipe = craft_get_session_recipe( sess );
    if ( !sess || !recipe )
    {
        err = "No recipe selected.";
        return false;
    }

    tmpl.custom_name = sess->custom_name;
    tmpl.extra_arg   = sess->extra_arg;

    if ( sess->base_material )
    {
        if ( sess->base_material->name )
            tmpl.base_name = sess->base_material->name;

        if ( sess->base_material->short_descr )
            tmpl.base_short = sess->base_material->short_descr;
    }

    switch ( recipe->output_mode )
    {
        case CraftOutputMode::CreateFromPrototype:
            pObjIndex = get_obj_index( recipe->result_vnum );
            if ( !pObjIndex )
            {
                err = "The recipe result object is missing from the database.";
                return false;
            }

            result = create_object( pObjIndex, craft_skill_level(ch, recipe->gsn) );
            break;

        case CraftOutputMode::TransformBaseMaterial:
            if ( !sess->base_material )
            {
                err = "No base material has been selected.";
                return false;
            }

            result = sess->base_material;
            break;
    }

    result->item_type = recipe->result_item_type;

    for ( int flag : recipe->set_wear_flags )
        BV_SET_BIT( result->wear_flags, flag );

    craft_apply_extra_wearloc(sess, result);

    for ( int flag : recipe->set_objflags )
        BV_SET_BIT( result->objflags, flag );

    if ( !recipe->name_template.empty() )
    {
        STRFREE( result->name );
        result->name = STRALLOC( craft_expand_template( recipe->name_template, tmpl ) );
    }

    if ( !recipe->short_template.empty() )
    {
        STRFREE( result->short_descr );
        result->short_descr = STRALLOC( craft_expand_template( recipe->short_template, tmpl ) );
    }

    if ( !recipe->description_template.empty() )
    {
        STRFREE( result->description );
        result->description = STRALLOC( craft_expand_template( recipe->description_template, tmpl ) );
    }

    return true;
}

static bool craft_validate_slot_counts( CraftSession *sess, std::string &err )
{
    const CraftRecipe *recipe = craft_get_session_recipe( sess );
    if ( !sess || !recipe )
    {
        err = "No recipe selected.";
        return false;
    }
    if ( recipe->output_mode == CraftOutputMode::TransformBaseMaterial && !sess->base_material )
    {
        err = "You must choose a base material first.";
        return false;
    }
    for ( const auto &slot : recipe->slots )
    {
        int count = craft_count_selected( sess, &slot );

        if ( count < slot.min_count )
        {
            err = str_printf( "You still need %s.", slot.name );
            return false;
        }

        if ( slot.max_count > 0 && count > slot.max_count )
        {
            err = str_printf( "Too many items selected for %s.", slot.name );
            return false;
        }
    }

    return true;
}

static bool craft_validate_session( CHAR_DATA *ch, CraftSession *sess, std::string &err )
{
    const CraftRecipe *recipe = craft_get_session_recipe( sess );
    if ( !sess || !recipe )
    {
        err = "No recipe selected.";
        return false;
    }

    if ( !craft_check_room( ch, recipe, err ) )
        return false;

    if ( !craft_validate_slot_counts( sess, err ) )
        return false;

    if ( recipe->allow_custom_name && sess->custom_name.empty() )
    {
        err = "You must name the item first.";
        return false;
    }

    if ( recipe->requires_extra_arg && sess->extra_arg.empty() )
    {
        err = str_printf( "You must set %s first.", recipe->extra_arg_name );
        return false;
    }

    return craft_recompute_vars( ch, sess, err );
}

static void craft_add_slot(CraftRecipe &r, const char *name, int item_type, int min_count,
                           int max_count, CraftConsumeMode mode, bool base_ok)
{
    r.slots.push_back(CraftSlotDef{ name, item_type, min_count, max_count, mode, base_ok });
}

static void craft_add_assignment( CraftRecipe &r, const char *target, CraftExpr expr )
{
    r.assignments.push_back( CraftAssignment{ target, std::move(expr) } );
}

static void craft_add_affect( CraftRecipe &r, int location, CraftExpr modifier_expr,
                              int bitvector = -1 )
{
    CraftAffectDef def;
    def.location = location;
    def.modifier_expr = std::move(modifier_expr);
    def.bitvector = bitvector;
    def.use_duration = false;

    r.affects.push_back( std::move(def) );
}

static void craft_add_derived_var( CraftRecipe &r, const char *var_name, const char *slot_name,
                                   const char *source_field, const char *aggregate_op,
                                   bool use_clamp = false, int clamp_min = 0,
                                   bool clamp_max_from_skill_mult = false, int clamp_max_skill_mult = 0,
                                   bool clamp_max_constant = false, int clamp_max_value = 0 )
{
    CraftDerivedVarDef def;
    def.var_name = var_name;
    def.slot_name = slot_name;
    def.source_field = source_field;
    def.aggregate_op = aggregate_op;
    def.use_clamp = use_clamp;
    def.clamp_min = clamp_min;
    def.clamp_max_from_skill_mult = clamp_max_from_skill_mult;
    def.clamp_max_skill_mult = clamp_max_skill_mult;
    def.clamp_max_constant = clamp_max_constant;
    def.clamp_max_value = clamp_max_value;

    r.derived_vars.push_back( std::move(def) );
}

static std::vector<CraftRecipe> build_craft_recipes()
{
    std::vector<CraftRecipe> out;

    {
        CraftRecipe armor;

        armor.gsn = gsn_makearmor;
        armor.name = "armor";
        armor.display_name = "Armor";
        armor.room_req = CraftRoomRequirement::Factory;
        armor.timer_ticks = 15;
        armor.output_mode = CraftOutputMode::TransformBaseMaterial;
        armor.result_vnum = 0;
        armor.result_item_type = ITEM_ARMOR;
        armor.allow_custom_name = true;
        armor.requires_extra_arg = true;
        armor.extra_arg_name = "wearloc";

        armor.set_wear_flags.push_back( ITEM_TAKE );

        armor.name_template = "{name}";
        armor.short_template = "{name}";
        armor.description_template = "{name} was dropped here.";

        craft_add_slot( armor, "fabric", ITEM_FABRIC, 1, 1, CraftConsumeMode::TransformBase, true );
        craft_add_slot( armor, "thread", ITEM_THREAD, 1, 1, CraftConsumeMode::Keep, false );

        craft_add_assignment( armor, "level", craft_skill() );
        craft_add_assignment( armor, "value0", craft_var("base_value1") );
        craft_add_assignment(
            armor,
            "cost",
            craft_binop( "mul", craft_var("base_cost"), craft_const(10) ) );

        out.push_back( std::move(armor) );
    }

    {
        CraftRecipe bowcaster;

        bowcaster.gsn = gsn_makebowcaster;
        bowcaster.name = "bowcaster";
        bowcaster.display_name = "Bowcaster";
        bowcaster.room_req = CraftRoomRequirement::Factory;
        bowcaster.timer_ticks = 25;
        bowcaster.output_mode = CraftOutputMode::CreateFromPrototype;
        bowcaster.result_vnum = 10431;
        bowcaster.result_item_type = ITEM_WEAPON;
        bowcaster.allow_custom_name = true;
        bowcaster.requires_extra_arg = false;
        bowcaster.extra_arg_name = "";

        bowcaster.set_wear_flags.push_back( ITEM_WIELD );
        bowcaster.set_wear_flags.push_back( ITEM_TAKE );

        bowcaster.set_objflags.push_back( WEAPON_BOWCASTER );

        bowcaster.name_template = "{name} bowcaster";
        bowcaster.short_template = "{name}";
        bowcaster.description_template = "{name} was carefully placed here.";

        craft_add_slot( bowcaster, "toolkit",   ITEM_TOOLKIT,   1, 1, CraftConsumeMode::Keep, false );
        craft_add_slot( bowcaster, "duraplast", ITEM_DURAPLAST, 1, 1, CraftConsumeMode::Consume, false );
        craft_add_slot( bowcaster, "crossbow",  ITEM_CROSSBOW,  1, 1, CraftConsumeMode::Consume, false );
        craft_add_slot( bowcaster, "oven",      ITEM_OVEN,      1, 1, CraftConsumeMode::Keep, false );
        craft_add_slot( bowcaster, "tinder",    ITEM_TINDER,    1, 4, CraftConsumeMode::Consume, false );
        craft_add_slot( bowcaster, "oil",       ITEM_OIL,       1, 1, CraftConsumeMode::Consume, false );
        craft_add_slot( bowcaster, "lens",      ITEM_LENS,      0, 2, CraftConsumeMode::Consume, false );
        craft_add_slot( bowcaster, "bolt",      ITEM_BOLT,      0, 1, CraftConsumeMode::Consume, false );

        craft_add_derived_var( bowcaster, "ammo",  "bolt",   "value0", "highest" );
        craft_add_derived_var( bowcaster, "power", "tinder", "value0", "count" );
        craft_add_derived_var( bowcaster, "scope", "lens",   "value0", "count" );

        craft_add_assignment( bowcaster, "level", craft_skill() );
        craft_add_assignment(
            bowcaster,
            "weight",
            craft_binop(
                "add",
                craft_const(2),
                craft_binop( "div", craft_skill(), craft_const(7) ) ) );
        craft_add_assignment( bowcaster, "value0", craft_const(INIT_WEAPON_CONDITION) );
        craft_add_assignment(
            bowcaster,
            "value1",
            craft_binop(
                "add",
                craft_binop( "div", craft_skill(), craft_const(10) ),
                craft_const(25) ) );
        craft_add_assignment(
            bowcaster,
            "value2",
            craft_binop(
                "add",
                craft_binop( "div", craft_skill(), craft_const(5) ),
                craft_const(25) ) );
        craft_add_assignment( bowcaster, "value4", craft_var("ammo") );
        craft_add_assignment( bowcaster, "value5", craft_const(250) );

        craft_add_affect(
            bowcaster,
            APPLY_HITROLL,
            craft_binop(
                "min",
                craft_binop(
                    "add",
                    craft_const(1),
                    craft_var("scope")
                ),
                craft_binop(
                    "div",
                    craft_skill(),
                    craft_const(30)
                )
            )
        );

        craft_add_affect(
            bowcaster,
            APPLY_DAMROLL,
            craft_binop(
                "min",
                craft_var("power"),
                craft_binop(
                    "div",
                    craft_skill(),
                    craft_const(30)
                )
            )
        );
        out.push_back( std::move(bowcaster) );
    }

    return out;
}

static std::vector<CraftRecipe> &craft_get_recipes()
{
    static std::vector<CraftRecipe> recipes;
    static bool loaded = false;

    if ( !loaded )
    {
        if ( !load_all_craft_recipes( recipes ) )
            recipes = build_craft_recipes();

        loaded = true;
    }

    return recipes;
}

static const CraftRecipe *craft_get_session_recipe( const CraftSession *sess )
{
    if ( !sess || sess->recipe_name.empty() )
        return nullptr;

    for ( const auto &recipe : craft_get_recipes() )
    {
        if ( !str_cmp( sess->recipe_name, recipe.name ) )
            return &recipe;
    }

    return nullptr;
}

static bool craft_expr_contains_var( const CraftExpr &expr, const std::string &name )
{
    if ( !str_cmp( expr.op, "var" ) && expr.var_name == name )
        return true;

    if ( expr.left && craft_expr_contains_var( *expr.left, name ) )
        return true;

    if ( expr.right && craft_expr_contains_var( *expr.right, name ) )
        return true;

    return false;
}

static const char *craft_get_base_var_label( CraftSession *sess, const std::string &var_name )
{
    const OlcItemValueInfo *info;
    int idx = -1;

    if ( !sess || !sess->base_material )
        return nullptr;

    if ( var_name == "base_value0" ) idx = 0;
    else if ( var_name == "base_value1" ) idx = 1;
    else if ( var_name == "base_value2" ) idx = 2;
    else if ( var_name == "base_value3" ) idx = 3;
    else if ( var_name == "base_value4" ) idx = 4;
    else if ( var_name == "base_value5" ) idx = 5;
    else if ( var_name == "base_cost" )   return "cost";
    else if ( var_name == "base_weight" ) return "weight";
    else if ( var_name == "base_level" )  return "level";
    else
        return nullptr;

    info = craft_get_item_value_info( sess->base_material->item_type );
    if ( !info || !info->values_used || idx < 0 || idx > 5 )
        return nullptr;

    return info->labels[idx];
}

static void craft_show_session( CHAR_DATA *ch )
{
    CraftSession *sess = craft_get_session( ch );
    std::string err;

    const CraftRecipe *recipe = craft_get_session_recipe( sess );
    if ( !sess || !recipe )
    {
        send_to_char( "No recipe selected.\n", ch );
        return;
    }

    craft_recompute_vars( ch, sess, err );

    ch_printf( ch, "&YRecipe:&w %s\n", recipe->display_name.c_str() );
    ch_printf( ch, "&YName:&w   %s\n", sess->custom_name.empty() ? "(unset)" : sess->custom_name.c_str() );

    if ( recipe->requires_extra_arg )
        ch_printf( ch, "&Y%s:&w %s\n",
            recipe->extra_arg_name.c_str(),
            sess->extra_arg.empty() ? "(unset)" : sess->extra_arg.c_str() );

    if ( sess->base_material )
        ch_printf( ch, "&YBase:&w   %s\n", sess->base_material->short_descr );
    else
        ch_printf( ch, "&YBase:&w   (none)\n" );

    send_to_char( "&YSlots:&w\n", ch );
    for ( const auto &slot : recipe->slots )
    {
        int count = craft_count_selected( sess, &slot );

        ch_printf( ch, "  %-12s %d", slot.name.c_str(), count );

        if ( slot.max_count > 0 )
            ch_printf( ch, "/%d", slot.max_count );

        ch_printf( ch,
            "  [%s]",
            slot.min_count == 0 ? "optional" : "required" );

        if ( slot.consume_mode == CraftConsumeMode::Keep )
            send_to_char( " [kept]", ch );
        else if ( slot.consume_mode == CraftConsumeMode::Consume )
            send_to_char( " [consumed]", ch );
        else if ( slot.consume_mode == CraftConsumeMode::TransformBase )
            send_to_char( " [base]", ch );

        send_to_char( "\n", ch );
    }

    if ( !sess->vars.empty() )
    {
        const OlcItemValueInfo *info = craft_get_item_value_info( recipe->result_item_type );

        send_to_char( "\n&YDerived values:&w\n", ch );
        for ( const auto &it : sess->vars )
        {
            const char *base_label = craft_get_base_var_label( sess, it.first );

            if ( base_label && base_label[0] != '\0' )
            {
                ch_printf( ch, "  %s (%s): %d\n",
                    it.first.c_str(),
                    base_label,
                    it.second );
                continue;
            }

            bool shown = false;

            for ( const auto &assign : recipe->assignments )
            {
                if ( craft_expr_contains_var( assign.expr, it.first ) )
                {
                    int idx = craft_target_to_value_index( assign.target );

                    if ( idx >= 0 && info && info->values_used && info->labels[idx] )
                    {
                        ch_printf( ch, "  %s (%s): %d\n",
                            it.first.c_str(),
                            info->labels[idx],
                            it.second );
                        shown = true;
                        break;
                    }
                }
            }

            if ( !shown )
            {
                ch_printf( ch, "  %s: %d\n",
                    it.first.c_str(),
                    it.second );
            }
        }
    }

    send_to_char( "&YType 'pause' to leave the menu and resume later, or 'cancel' to discard it.&w\n", ch );        
}

static const CraftRecipe *craft_find_recipe( const std::string &name )
{
    for ( const auto &recipe : craft_get_recipes() )
        if ( !str_prefix( name, recipe.name ) )
            return &recipe;
    return nullptr;
}

static void craft_show_recipe_list( CHAR_DATA *ch )
{
    send_to_char( "&YCraftable Recipes&w\n", ch );

    for ( const auto &recipe : craft_get_recipes() )
    {
        if ( !craft_knows_recipe( ch, &recipe ) )
            continue;
        if ( recipe.disabled )
            continue;            

        ch_printf( ch, "  %-16s (%d%%)\n",
            recipe.display_name.c_str(),
            craft_skill_level( ch, recipe.gsn ) );
    }
}

static const char *craft_bool_word( bool v )
{
    return v ? "yes" : "no";
}

static void craft_dump_recipe( CHAR_DATA *ch, const CraftRecipe *recipe )
{
    if ( !ch || !recipe )
    {
        send_to_char( "No such recipe.\n", ch );
        return;
    }

    ch_printf( ch, "&YCraft Recipe Dump&w\n" );
    ch_printf( ch, "  Name:             %s\n", recipe->name.c_str() );
    ch_printf( ch, "  Display Name:     %s\n", recipe->display_name.c_str() );
    ch_printf( ch, "  GSN:              %s\n", craft_gsn_name( recipe->gsn ) );
    ch_printf( ch, "  Room Req:         %s\n", craft_room_req_name( recipe->room_req ) );
    ch_printf( ch, "  Timer Ticks:      %d\n", recipe->timer_ticks );
    ch_printf( ch, "  Output Mode:      %s\n", craft_output_mode_name( recipe->output_mode ) );
    ch_printf( ch, "  Result Vnum:      %d\n", recipe->result_vnum );
    ch_printf( ch, "  Result Type:      %s\n", craft_item_type_name( recipe->result_item_type ) );
    ch_printf( ch, "  Allow Name:       %s\n", craft_bool_word( recipe->allow_custom_name ) );
    ch_printf( ch, "  Requires Extra:   %s\n", craft_bool_word( recipe->requires_extra_arg ) );
    ch_printf( ch, "  Extra Arg Name:   %s\n", recipe->extra_arg_name.empty() ? "(none)" : recipe->extra_arg_name.c_str() );
    ch_printf( ch, "  Name Template:    %s\n", recipe->name_template.empty() ? "(none)" : recipe->name_template.c_str() );
    ch_printf( ch, "  Short Template:   %s\n", recipe->short_template.empty() ? "(none)" : recipe->short_template.c_str() );
    ch_printf( ch, "  Desc Template:    %s\n", recipe->description_template.empty() ? "(none)" : recipe->description_template.c_str() );

    send_to_char( "\n&YWear Flags:&w\n", ch );
    if ( recipe->set_wear_flags.empty() )
    {
        send_to_char( "  (none)\n", ch );
    }
    else
    {
        for ( int flag : recipe->set_wear_flags )
        {   
            ch_printf( ch, "  %s\n",  get_flag_name(w_flags, flag, WEAR_MAX) );
        }
    }

    send_to_char( "\n&YObject Flags:&w\n", ch );
    if ( recipe->set_objflags.empty() )
    {
        send_to_char( "  (none)\n", ch );
    }
    else
    {
        for ( int flag : recipe->set_objflags )
            ch_printf( ch, "  %s\n",  get_flag_name(obj_flag_table, flag, ITEM_MAX) );
    }

    send_to_char( "\n&YSlots:&w\n", ch );
    if ( recipe->slots.empty() )
    {
        send_to_char( "  (none)\n", ch );
    }
    else
    {
        int i = 0;
        for ( const auto &slot : recipe->slots )
        {
            ++i;
            ch_printf( ch,
                "  [%d] name=%s type=%s min=%d max=%d optional=%s consume=%s base_ok=%s\n",
                i,
                slot.name.c_str(),
                craft_item_type_name( slot.item_type ),
                slot.min_count,
                slot.max_count,
                slot.min_count == 0 ? "optional" : "required",
                craft_consume_mode_name( slot.consume_mode ),
                craft_bool_word( slot.may_be_base_material ) );
        }
    }

    send_to_char( "\n&YDerived Vars:&w\n", ch );
    if ( recipe->derived_vars.empty() )
    {
        send_to_char( "  (none)\n", ch );
    }
    else
    {
        int i = 0;
        for ( const auto &dv : recipe->derived_vars )
        {
            ++i;
            ch_printf( ch,
                "  [%d] var=%s slot=%s field=%s op=%s clamp=%s min=%d max_skill=%s mult=%d max_const=%s value=%d\n",
                i,
                dv.var_name.c_str(),
                dv.slot_name.c_str(),
                dv.source_field.c_str(),
                dv.aggregate_op.c_str(),
                craft_bool_word( dv.use_clamp ),
                dv.clamp_min,
                craft_bool_word( dv.clamp_max_from_skill_mult ),
                dv.clamp_max_skill_mult,
                craft_bool_word( dv.clamp_max_constant ),
                dv.clamp_max_value );
        }
    }

    send_to_char( "\n&YAssignments:&w\n", ch );
    if ( recipe->assignments.empty() )
    {
        send_to_char( "  (none)\n", ch );
    }
    else
    {
        int i = 0;
        for ( const auto &asgn : recipe->assignments )
        {
            ++i;
            ch_printf( ch,
                "  [%d] %s = %s\n",
                i,
                asgn.target.c_str(),
                craft_expr_to_string( asgn.expr ).c_str() );
        }
    }

    send_to_char( "\n&YAffects:&w\n", ch );
    if ( recipe->affects.empty() )
    {
        send_to_char( "  (none)\n", ch );
    }
    else
    {
        int i = 0;
        for ( const auto &aff : recipe->affects )
        {
            ++i;
            ch_printf( ch,
                "  [%d] location=%d modifier=%s bitvector=%d use_duration=%s duration=%s\n",
                i,
                aff.location,
                craft_expr_to_string( aff.modifier_expr ).c_str(),
                aff.bitvector,
                craft_bool_word( aff.use_duration ),
                craft_expr_to_string( aff.duration_expr ).c_str() );
        }
    }
}

static void craft_cmd_dump( CHAR_DATA *ch, const std::string &arg )
{
    const CraftRecipe *recipe;

    if ( arg.empty() )
    {
        send_to_char( "Usage: dump <recipe>\n", ch );
        return;
    }

    recipe = craft_find_recipe( arg );
    if ( !recipe )
    {
        send_to_char( "No such recipe.\n", ch );
        return;
    }

    craft_dump_recipe( ch, recipe );
}

static void craft_cmd_unuse( CHAR_DATA *ch, const std::string &slot_name, const std::string &item_name )
{
    CraftSession *sess = craft_get_session( ch );

    const CraftRecipe *recipe = craft_get_session_recipe( sess );
    if ( !sess || !recipe )
    {
        send_to_char( "No recipe selected.\n", ch );
        return;
    }

    if ( slot_name.empty() || item_name.empty() )
    {
        send_to_char( "Usage: unuse <slot> <item>\n", ch );
        return;
    }

    for ( auto it = sess->selected.begin(); it != sess->selected.end(); ++it )
    {
        if ( !str_cmp( it->slot_name, slot_name )
        && it->obj
        && nifty_is_name( item_name, it->obj->name ) )
        {
            if ( sess->base_material == it->obj )
                sess->base_material = nullptr;

            ch_printf( ch, "You remove %s from %s.\n", it->obj->short_descr, it->slot_name.c_str() );
            sess->selected.erase( it );
            return;
        }
    }

    send_to_char( "That item is not assigned to that slot.\n", ch );
}

static bool craft_try_select_recipe( CHAR_DATA *ch, const std::string &arg, bool show_errors )
{
    CraftSession *sess;
    const CraftRecipe *recipe = craft_find_recipe( arg );

    if ( arg.empty() )
    {
        if ( show_errors )
            craft_show_recipe_list( ch );
        return false;
    }

    if ( !recipe )
    {
        if ( show_errors )
            send_to_char( "No such recipe.\n", ch );
        return false;
    }

    if ( recipe->disabled )
    {
        if ( show_errors )
            send_to_char( "That recipe is currently disabled.\n", ch );
        return false;
    }

    if ( !craft_knows_recipe( ch, recipe ) )
    {
        if ( show_errors )
            send_to_char( "You have not learned that recipe.\n", ch );
        return false;
    }

    sess = craft_get_session( ch );
    if ( !sess )
        sess = craft_create_session( ch );

    *sess = CraftSession();
    sess->recipe_name = recipe->name;

    ch_printf( ch, "You select %s.\n", recipe->display_name.c_str() );
    craft_show_session( ch );
    return true;
}

static bool craft_find_auto_item_for_slot( CHAR_DATA *ch, CraftSession *sess,
                                           const CraftSlotDef &slot, OBJ_DATA *&out )
{
    OBJ_DATA *obj;

    out = nullptr;

    if ( !ch || !sess )
        return false;

    for ( obj = ch->last_carrying; obj; obj = obj->prev_content )
    {
        if ( obj->item_type != slot.item_type )
            continue;

        if ( craft_count_selected_object( sess, obj ) >= obj->count )
            continue;

        out = obj;
        return true;
    }

    return false;
}

static bool craft_try_autoselect( CHAR_DATA *ch, std::string &err )
{
    CraftSession *sess = craft_get_session( ch );
    OBJ_DATA *first_base_candidate = nullptr;

    const CraftRecipe *recipe = craft_get_session_recipe( sess );
    if ( !sess || !recipe )
    {
        err = "No recipe selected.";
        return false;
    }

    /* clear current picks so autoselect is deterministic */
    sess->selected.clear();
    sess->base_material = nullptr;

    for ( const auto &slot : recipe->slots )
    {
        while ( slot.max_count == 0 || craft_count_selected( sess, &slot ) < slot.min_count )
        {
            OBJ_DATA *obj = nullptr;

            if ( !craft_find_auto_item_for_slot( ch, sess, slot, obj ) )
            {
                err = str_printf( "You do not have enough items for slot '%s'.", slot.name.c_str() );
                sess->selected.clear();
                sess->base_material = nullptr;
                return false;
            }

            sess->selected.push_back( { slot.name, obj } );

            if ( slot.may_be_base_material && !first_base_candidate )
                first_base_candidate = obj;
        }
    }

    if ( recipe->output_mode == CraftOutputMode::TransformBaseMaterial )
    {
        if ( first_base_candidate )
            sess->base_material = first_base_candidate;
        else
        {
            err = "No eligible base material was found.";
            sess->selected.clear();
            return false;
        }
    }

    return true;
}

static void craft_cmd_select( CHAR_DATA *ch, const std::string &arg )
{
    std::string rest;
    std::string first;
    std::string err;

    rest = one_argument( arg, first );

    if ( first.empty() )
    {
        craft_show_recipe_list( ch );
        return;
    }

    if ( !str_prefix( first, "autoselect" ) )
    {
        if ( !craft_try_autoselect( ch, err ) )
        {
            send_to_char( err, ch );
            send_to_char( "\n", ch );
            return;
        }

        send_to_char( "You automatically assign materials from your inventory.\n", ch );
        craft_show_session( ch );
        return;
    }

    craft_try_select_recipe( ch, arg, true );
}

static void craft_cmd_name( CHAR_DATA *ch, const std::string &arg )
{
    CraftSession *sess = craft_get_session( ch );

    const CraftRecipe *recipe = craft_get_session_recipe( sess );
    if ( !sess || !recipe )
    {
        send_to_char( "No recipe selected.\n", ch );
        return;
    }

    if ( arg.empty() )
    {
        send_to_char( "Name it what?\n", ch );
        return;
    }

    sess->custom_name = arg;
    ch_printf( ch, "Name set to: %s\n", sess->custom_name.c_str() );
}

static void craft_cmd_set( CHAR_DATA *ch, const std::string &field, const std::string &value )
{
    CraftSession *sess = craft_get_session( ch );

    const CraftRecipe *recipe = craft_get_session_recipe( sess );
    if ( !sess || !recipe )
    {
        send_to_char( "No recipe selected.\n", ch );
        return;
    }

    if ( field.empty() || value.empty() )
    {
        send_to_char( "Usage: set <field> <value>\n", ch );
        return;
    }

    if ( recipe->requires_extra_arg && !str_cmp( field, recipe->extra_arg_name ) )
    {
        if ( !str_cmp( recipe->extra_arg_name, "wearloc" ) )
        {
            int wearflag = get_wflag( value );

            if ( wearflag < 0 )
            {
                send_to_char( "Unknown wear location.\n", ch );
                return;
            }

            if ( craft_recipe_blocks_wearloc( recipe, wearflag ) )
            {
                send_to_char( "That wear location is not allowed for this recipe.\n", ch );
                return;
            }
        }
        if ( !str_cmp( recipe->extra_arg_name, "sexrace" ) )
        {
            int sex = 0, race = 0;

            if ( !craft_parse_sexrace( value, sex, race ) )
            {
                send_to_char( "Usage: set sexrace <sex 0-2> <race 0-33>\n", ch );
                return;
            }

            // Store parsed values as session variables
            sess->vars["sex"] = sex;
            sess->vars["race"] = race;
        }
        sess->extra_arg = value;
        ch_printf( ch, "%s set to %s.\n", field.c_str(), value.c_str() );
        return;
    }

    send_to_char( "That field is not used by this recipe.\n", ch );
}

static void craft_cmd_use( CHAR_DATA *ch, const std::string &slot_name, const std::string &item_name )
{
    CraftSession *sess = craft_get_session( ch );
    const CraftSlotDef *slot;
    OBJ_DATA *obj;

    const CraftRecipe *recipe = craft_get_session_recipe( sess );
    if ( !sess || !recipe )
    {
        send_to_char( "No recipe selected.\n", ch );
        return;
    }

    if ( slot_name.empty() || item_name.empty() )
    {
        send_to_char( "Usage: use <slot> <item>\n", ch );
        return;
    }

    slot = craft_find_slot( recipe, slot_name );
    if ( !slot )
    {
        send_to_char( "No such slot for this recipe.\n", ch );
        return;
    }

    obj = get_obj_carry( ch, item_name );
    if ( !obj )
    {
        send_to_char( "You do not have that item.\n", ch );
        return;
    }

    if ( obj->item_type != slot->item_type )
    {
        send_to_char( "That item does not fit that slot.\n", ch );
        return;
    }

    if ( slot->max_count > 0 && craft_count_selected( sess, slot ) >= slot->max_count )
    {
        send_to_char( "That slot is already full.\n", ch );
        return;
    }

    if ( craft_count_selected_object( sess, obj ) >= obj->count )
    {
        send_to_char( "You have already assigned all of that item.\n", ch );
        return;
    }

    sess->selected.push_back( { slot->name, obj } );

    if ( slot->consume_mode == CraftConsumeMode::TransformBase && slot->may_be_base_material && !sess->base_material )
        sess->base_material = obj;

    ch_printf( ch, "You assign %s to %s.\n", obj->short_descr, slot->name.c_str() );
}

static void craft_cmd_base( CHAR_DATA *ch, const std::string &item_name )
{
    CraftSession *sess = craft_get_session( ch );
    OBJ_DATA *obj = get_obj_carry( ch, item_name );

    const CraftRecipe *recipe = craft_get_session_recipe( sess );
    if ( !sess || !recipe )
    {
        send_to_char( "No recipe selected.\n", ch );
        return;
    }

    if ( recipe->output_mode != CraftOutputMode::TransformBaseMaterial )
    {
        send_to_char( "This recipe does not use a base material.\n", ch );
        return;
    }

    if ( !obj )
    {
        send_to_char( "You do not have that item.\n", ch );
        return;
    }

    for ( const auto &sel : sess->selected )
    {
        const CraftSlotDef *slot = craft_get_selected_slot_def( sess, sel );

        if ( sel.obj == obj && slot && slot->may_be_base_material )
        {
            sess->base_material = obj;
            ch_printf( ch, "Base material set to %s.\n", obj->short_descr );
            return;
        }
    }

    send_to_char( "That item is not an eligible base material for this recipe.\n", ch );
}

static bool craft_prepare_selected_items( CraftSession *sess, std::string &err )
{
    std::vector<OBJ_DATA*> prepared;

    const CraftRecipe *recipe = craft_get_session_recipe( sess );
    if ( !sess || !recipe )
    {
        err = "No recipe selected.";
        return false;
    }

    auto already = [&]( OBJ_DATA *obj ) -> bool
    {
        for ( OBJ_DATA *p : prepared )
            if ( p == obj )
                return true;
        return false;
    };

    for ( auto &sel : sess->selected )
    {
        const CraftSlotDef *slot = craft_get_selected_slot_def( sess, sel );

        if ( !slot || !sel.obj )
            continue;

        if ( slot->consume_mode == CraftConsumeMode::Consume
        || slot->consume_mode == CraftConsumeMode::TransformBase )
        {
            if ( !already( sel.obj ) )
            {
                separate_obj( sel.obj );
                prepared.push_back( sel.obj );
            }
        }
    }

    if ( sess->base_material && !already( sess->base_material ) )
        separate_obj( sess->base_material );

    return true;
}

static bool craft_consume_input_items( CraftSession *sess, std::string &err )
{
    std::vector<OBJ_DATA*> consumed;

    const CraftRecipe *recipe = craft_get_session_recipe( sess );
    if ( !sess || !recipe )
    {
        err = "No recipe selected.";
        return false;
    }

    for ( const auto &sel : sess->selected )
    {
        const CraftSlotDef *slot = craft_get_selected_slot_def( sess, sel );

        if ( !slot || !sel.obj )
            continue;

        if ( slot->consume_mode == CraftConsumeMode::Consume )
            consumed.push_back( sel.obj );
    }

    for ( OBJ_DATA *obj : consumed )
    {
        separate_obj( obj );
        obj_from_char( obj );
        extract_obj( obj );
    }

    return true;
}

static void craft_discard_failed_result( CraftSession *sess, OBJ_DATA *result )
{
    const CraftRecipe *recipe = craft_get_session_recipe( sess );
    if ( !sess || !recipe || !result )    
        return;

    if ( recipe->output_mode == CraftOutputMode::CreateFromPrototype )
        extract_obj( result );
}

static void craft_cleanup_session_after_build( CraftSession *sess, OBJ_DATA *result )
{
    const CraftRecipe *recipe = craft_get_session_recipe( sess );

    if ( !sess || !recipe )
        return;

    for ( auto it = sess->selected.begin(); it != sess->selected.end(); )
    {
        const CraftSlotDef *slot = craft_find_slot( recipe, it->slot_name );

        if ( !slot || !it->obj )
        {
            it = sess->selected.erase( it );
            continue;
        }

        switch ( slot->consume_mode )
        {
            case CraftConsumeMode::Keep:
                ++it;
                break;

            case CraftConsumeMode::Consume:
                it = sess->selected.erase( it );
                break;

            case CraftConsumeMode::TransformBase:
                if ( it->obj == result )
                    it = sess->selected.erase( it );
                else
                    ++it;
                break;
        }
    }

    if ( sess->base_material == result )
        sess->base_material = nullptr;

    sess->vars.clear();
}

void do_craft_build( CHAR_DATA *ch, char *argument )
{
    CraftSession *sess = craft_get_session( ch );
    std::string err;
    OBJ_DATA *result = nullptr;
    int chance;

    switch ( ch->substate )
    {
        default:
            return;

        case SUB_TIMER_DO_ABORT:
            ch->substate = SUB_CRAFT_MENU;
            send_to_char( "You are interrupted and fail to finish your work.\n", ch );
            craft_show_session( ch );
            return;

        case 1:
            break;
    }

    ch->substate = SUB_CRAFT_MENU;

    const CraftRecipe *recipe = craft_get_session_recipe( sess );
    if ( !sess || !recipe )
    {
        send_to_char( "Your crafting session has been lost.\n", ch );
        return;
    }

    if (!craft_selected_items_still_valid(ch, sess, err))
    {
        send_to_char( err, ch );
        send_to_char( "\n", ch );
        craft_show_session( ch );
        return;
    }

    if ( !craft_validate_session( ch, sess, err ) )
    {
        send_to_char( err, ch );
        send_to_char( "\n", ch );
        craft_show_session( ch );
        return;
    }

    if ( !craft_prepare_selected_items( sess, err ) )
    {
        send_to_char( err, ch );
        send_to_char( "\n", ch );
        learn_from_failure( ch, recipe->gsn );
        craft_show_session( ch );
        return;
    }

    if ( !craft_consume_input_items( sess, err ) )
    {
        send_to_char( err, ch );
        send_to_char( "\n", ch );
        learn_from_failure( ch, recipe->gsn );
        craft_show_session( ch );
        return;
    }

    chance = craft_skill_level( ch, recipe->gsn );
    if ( number_percent() > chance * 2 )
    {
        send_to_char( "Your work falls apart at the last moment.\n", ch );
        learn_from_failure( ch, recipe->gsn );
        craft_show_session( ch );
        return;
    }

    if ( !craft_create_result_object( ch, sess, result, err ) )
    {
        send_to_char( err, ch );
        send_to_char( "\n", ch );
        learn_from_failure( ch, recipe->gsn );
        craft_show_session( ch );
        return;
    }

    if ( !craft_apply_assignments( ch, sess, result, err ) )
    {
        send_to_char( err, ch );
        send_to_char( "\n", ch );
        craft_discard_failed_result( sess, result );
        learn_from_failure( ch, recipe->gsn );
        craft_show_session( ch );
        return;
    }

    if ( !craft_apply_affects( ch, sess, result, err ) )
    {
        send_to_char( err, ch );
        send_to_char( "\n", ch );
        craft_discard_failed_result( sess, result );
        learn_from_failure( ch, recipe->gsn );
        craft_show_session( ch );        
        return;
    }

    if ( result && result != sess->base_material )
        obj_to_char( result, ch );

    send_to_char( "You finish your work.\n", ch );
    act( AT_PLAIN, "$n finishes a crafting project.", ch, NULL, NULL, TO_ROOM );

    {
         long xpgain;
         
         xpgain = UMIN( result->cost*100 ,( exp_level(ch->skill_level[ENGINEERING_ABILITY]+1) - exp_level(ch->skill_level[ENGINEERING_ABILITY]) ) );
         gain_exp(ch, xpgain, ENGINEERING_ABILITY);
         ch_printf( ch , "You gain %ld engineering experience.", xpgain );
    }    
    learn_from_success( ch, recipe->gsn );
    craft_cleanup_session_after_build( sess, result );
    craft_show_session( ch );
}

static void craft_begin_build( CHAR_DATA *ch )
{
    CraftSession *sess = craft_get_session( ch );
    std::string err;
    int chance;

    const CraftRecipe *recipe = craft_get_session_recipe( sess );
    if ( !sess || !recipe )
    {
        send_to_char( "No recipe selected.\n", ch );
        return;
    }

    if ( !craft_validate_session( ch, sess, err ) )
    {
        send_to_char( err, ch );
        send_to_char( "\n", ch );
        return;
    }

    chance = craft_skill_level( ch, recipe->gsn );
    if ( number_percent() >= chance )
    {
        send_to_char( "You cannot figure out how to fit the parts together.\n", ch );
        learn_from_failure( ch, recipe->gsn );
        return;
    }

    ch_printf( ch, "You begin working on %s.\n", recipe->display_name.c_str() );
    act( AT_PLAIN, "$n begins working carefully on a crafting project.", ch, NULL, NULL, TO_ROOM );

//    ch->substate = SUB_NONE;

    add_timer( ch, TIMER_DO_FUN, recipe->timer_ticks, do_craft_build, 1 );
}

static void craft_pause_mode( CHAR_DATA *ch )
{
    if ( !ch )
        return;

    ch->substate = SUB_NONE;
}

bool craft_parse_expr_for_olc( const std::string &text, CraftExpr &out, std::string &err )
{
    return craft_expr_from_string( text, out, err );
}

std::string craft_expr_to_string_for_olc( const CraftExpr &expr )
{
    return craft_expr_to_string( expr );
}

bool craft_interpret( CHAR_DATA *ch, const std::string& argument )
{
    std::string cmd;
    std::string rest;

    rest = one_argument( argument, cmd );

    if ( cmd.empty() )
    {
        craft_show_session( ch );
        return true;
    }

    if ( !str_cmp( cmd, "help" ) )
    {
        std::string topic;
        one_argument( rest, topic );
        craft_show_help( ch, topic );
        return true;
    }

    if ( !str_cmp( cmd, "exprtest" ) )
    {
        craft_debug_expr_test( ch, rest );
        return true;
    }

    if ( !str_cmp( cmd, "savercps" ) )
    {
        save_all_craft_recipes();
        send_to_char( "Craft recipes saved.\n", ch );
        return true;
    }    

    if ( !str_cmp( cmd, "dump" ) )
    {
        craft_cmd_dump( ch, rest );
        return true;
    }

    if ( !str_cmp( cmd, "list" ) )
    {
        craft_show_recipe_list( ch );
        return true;
    }

    if ( !str_cmp( cmd, "select" ) )
    {
        craft_cmd_select( ch, rest );
        return true;
    }

    if ( !str_cmp( cmd, "unuse" ) )
    {
        std::string slot;
        std::string item;

        item = one_argument( rest, slot );
        craft_cmd_unuse( ch, slot, item );
        return true;
    }

    if ( !str_cmp( cmd, "show" ) )
    {
        craft_show_session( ch );
        return true;
    }

    if ( !str_cmp( cmd, "name" ) )
    {
        craft_cmd_name( ch, rest );
        return true;
    }

    if ( !str_cmp( cmd, "set" ) )
    {
        std::string field;
        std::string value;

        value = one_argument( rest, field );
        craft_cmd_set( ch, field, value );
        return true;
    }

    if ( !str_cmp( cmd, "use" ) )
    {
        std::string slot;
        std::string item;

        item = one_argument( rest, slot );
        craft_cmd_use( ch, slot, item );
        return true;
    }

    if ( !str_cmp( cmd, "base" ) )
    {
        craft_cmd_base( ch, rest );
        return true;
    }

    if ( !str_cmp( cmd, "build" ) )
    {
        craft_begin_build( ch );
        return true;
    }

    if ( !str_cmp( cmd, "pause" ) )
    {
        send_to_char( "You pause your crafting session.\n", ch );
        craft_pause_mode( ch );
        return true;
    }

    if ( !str_cmp( cmd, "cancel" ) || !str_cmp( cmd, "exit" ) )
    {
        send_to_char( "You leave the crafting interface.\n", ch );
        craft_exit_mode( ch );
        return true;
    }

    return false;
}

void do_craft( CHAR_DATA *ch, char *argument )
{
    std::string arg = argument ? argument : "";

    if ( IS_NPC(ch) )
        return;

    if ( !craft_get_session( ch ) )
    {
        craft_enter_mode( ch );
        send_to_char( "You enter crafting mode. Type 'help' for commands.\n", ch );

        if ( !arg.empty() )
        {
            /* first try normal craft subcommands */
            if ( craft_interpret( ch, arg ) )
                return;

            /* if not a craft command, treat it as a recipe name */
            if ( craft_try_select_recipe( ch, arg, false ) )
                return;

            send_to_char( "That is not a crafting command or recipe.\n", ch );
            craft_show_recipe_list( ch );
            return;
        }

        craft_show_recipe_list( ch );
        return;
    }

    if ( ch->substate != SUB_CRAFT_MENU )
    {
        ch->substate = SUB_CRAFT_MENU;
        send_to_char( "You resume your crafting session.\n", ch );
    }

    if ( arg.empty() )
    {
        craft_show_session( ch );
        return;
    }

    if ( !craft_interpret( ch, arg ) )
    {
        /* while already in craft mode, also allow bare recipe names */
        if ( craft_try_select_recipe( ch, arg, false ) )
            return;

        send_to_char( "That is not a crafting command. Type 'help' for crafting help, or 'cancel' to leave crafting mode.\n", ch );
    }
}



