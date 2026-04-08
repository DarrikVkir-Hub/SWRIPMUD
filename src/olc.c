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
    // DO_COMMANDS DO_?EDIT


#include <typeinfo>
#include <cstring>
#include "mud.h"
#include "olc.h"

// External functions
extern size_t visible_length(const char *txt);
extern char *wrap_text_ex(const char *txt, int width, int flags, int indent);
extern int get_exflag( char *flag );
extern int get_risflag( char *flag );
extern int get_npc_race( char *type );
extern size_t get_langflag(char *flag);
extern bool command_is_authorized_for_char(CHAR_DATA* ch, CMDTYPE* cmd);

// Internal functions
bool olc_extradescs_equal(const EXTRA_DESCR_DATA* a, const EXTRA_DESCR_DATA* b);
bool olc_affects_equal(const AFFECT_DATA* a, const AFFECT_DATA* b);
void olc_apply_flag_delta(BitSet& dst, const BitSet& edited, const BitSet& baseline);

void olc_show_mob(CHAR_DATA* ch, CHAR_DATA* mob, bool show_help, int term_width);
bool olc_mobile_edit_revert(CHAR_DATA* ch);
void olc_mobile_reset_pending_proto(OlcSession* sess);;
void olc_mobile_init_pending_proto_from_live(OlcSession* sess, CHAR_DATA* mob);
bool olc_mobile_maybe_sync_prototype(CHAR_DATA* live, const CHAR_DATA* baseline, const CHAR_DATA* edited);
bool olc_mobile_apply_pending_prototype_changes(CHAR_DATA* ch, CHAR_DATA* live);

void olc_show_object(CHAR_DATA* ch, OBJ_DATA* obj, bool help_only_mode, int term_width);
void olc_object_show_header(CHAR_DATA* ch, OBJ_DATA* obj);
bool olc_object_edit_revert(CHAR_DATA* ch);

void olc_show_room(CHAR_DATA* ch, ROOM_INDEX_DATA* room, bool show_help, int term_width);
void olc_show_extradesc_help(CHAR_DATA* ch);
void olc_show_affect_help(CHAR_DATA* ch);
bool olc_edit_affect_field_generic( CHAR_DATA* ch, void* target, olc_find_affect_by_number_fn find_affect_by_number,
    olc_parse_affect_value_fn parse_affect_value, olc_affect_list_ref_fn first_affect_ref, olc_affect_list_ref_fn last_affect_ref);



// --------------------------------------------
// GENERIC - generic format, string, flag handling that is not OLC specific
// --------------------------------------------
bool same_str(const char* a, const char* b)
{
    if (a == b)
        return true;

    if (!a)
        a = "";
    if (!b)
        b = "";

    return !str_cmp(a, b);
}

void olc_replace_string(char*& dst, const char* src)
{
    if (dst)
        STRFREE(dst);

    dst = (src && src[0] != '\0') ? STRALLOC((char*)src) : nullptr;
}

void olc_replace_string_idx(char*& dst, const char* src)
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

    const DirAlias aliases[] =
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

const char* exit_dir_label(int dir)
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

EXIT_DATA* find_exit_by_number(ROOM_INDEX_DATA* room, int index)
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

EXIT_DATA* find_exit_by_dir_index(ROOM_INDEX_DATA* room, int dir, int index)
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

EXIT_DATA* find_exit_by_keyword(ROOM_INDEX_DATA* room, const std::string& keyword)
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

bool olc_is_direction_alias(const char* cmd)
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

bool olc_str_eq(const char* a, const char* b)
{
    if (!a) a = "";
    if (!b) b = "";
    return !str_cmp(a, b);
}

std::vector<std::string> olc_wrap_value_pairs(
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
// OLC_FORMAT olc_format olc specific format handling
// --------------------------------------------
std::vector<std::string> olc_format_wrap_text(
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

std::vector<std::string> olc_format_line(
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

std::string  olc_format_exit_flags_list()
{
    std::vector<std::string> vals = flag_values_vec(ex_flags);
    if (vals.empty())
        return "";

    return olc_format_columns(vals, 80, 2);
}


bool olc_format_parse_strict_int(const std::string& s, int& out)
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

std::string olc_format_trim_left(std::string s)
{
    size_t pos = s.find_first_not_of(' ');
    if (pos == std::string::npos)
        return "";
    s.erase(0, pos);
    return s;
}

std::vector<std::string> olc_format_pending_exit_side_effects(
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

std::vector<std::string> olc_format_exit_list_lines(
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

std::string format_columns_for_field(
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

std::vector<std::string> format_multiline_block(
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

void olc_append_lines(
    std::vector<std::string>& dst,
    const std::vector<std::string>& src)
{
    dst.insert(dst.end(), src.begin(), src.end());
}

// --------------------------------------------
// OLC_GENERIC olc_generic functions
// --------------------------------------------

bool olc_affects_equal(const AFFECT_DATA* a, const AFFECT_DATA* b)
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

bool olc_extradescs_equal(const EXTRA_DESCR_DATA* a, const EXTRA_DESCR_DATA* b)
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

void olc_show_affect_location_choices(CHAR_DATA* ch)
{
    std::vector<std::string> values;

    if (!ch)
        return;

    for (size_t i = 0; a_types[i].name != nullptr; ++i)
    {
        int bit = a_types[i].bit;

        if (bit < 0 || bit >= MAX_APPLY_TYPE)
            continue;

        values.push_back(a_types[i].name);
    }

    send_to_char("Valid affect locations:\n", ch);

    if (!values.empty())
    {
        std::string col = olc_format_columns(
            values,
            ch->desc && ch->desc->term_width > 20 ? ch->desc->term_width : 75,
            4);
        send_to_char(col.c_str(), ch);
    }
}

enum class OlcAffectModifierKind
{
    NUMBER,
    AFF_FLAG,
    RIS_FLAG,
    SN
};

OlcAffectModifierKind olc_affect_modifier_kind(int loc)
{
    switch (loc)
    {
        case APPLY_AFFECT:
        case APPLY_REMOVE:
            return OlcAffectModifierKind::AFF_FLAG;

        case APPLY_RESISTANT:
        case APPLY_IMMUNE:
        case APPLY_SUSCEPTIBLE:
            return OlcAffectModifierKind::RIS_FLAG;

        case APPLY_WEAPONSPELL:
        case APPLY_WEARSPELL:
        case APPLY_REMOVESPELL:
        case APPLY_STRIPSN:
            return OlcAffectModifierKind::SN;

        default:
            return OlcAffectModifierKind::NUMBER;
    }
}

void olc_show_affect_modifier_choices(CHAR_DATA* ch, int loc)
{
    if (!ch)
        return;

    switch (olc_affect_modifier_kind(loc))
    {
        default:
        case OlcAffectModifierKind::NUMBER:
            send_to_char("This affect location requires a numeric modifier.\n", ch);
            return;

        case OlcAffectModifierKind::SN:
            send_to_char("This affect location requires an sn value.\n", ch);
            return;

        case OlcAffectModifierKind::AFF_FLAG:
        {
            std::vector<std::string> values;
            for (size_t i = 0; aff_flags[i].name != nullptr; ++i)
                values.push_back(aff_flags[i].name);

            send_to_char("Valid affect modifiers:\n", ch);

            if (!values.empty())
            {
                std::string col = olc_format_columns(
                    values,
                    ch->desc && ch->desc->term_width > 20 ? ch->desc->term_width : 75,
                    4);
                send_to_char(col.c_str(), ch);
            }
            return;
        }

        case OlcAffectModifierKind::RIS_FLAG:
        {
            std::vector<std::string> values;
            for (size_t i = 0; ris_flags[i].name != nullptr; ++i)
                values.push_back(ris_flags[i].name);

            send_to_char("Valid affect modifiers:\n", ch);

            if (!values.empty())
            {
                std::string col = olc_format_columns(
                    values,
                    ch->desc && ch->desc->term_width > 20 ? ch->desc->term_width : 75,
                    4);
                send_to_char(col.c_str(), ch);
            }
            return;
        }
    }
}



bool olc_edit_affect_field_generic(
    CHAR_DATA* ch,
    void* target,
    olc_find_affect_by_number_fn find_affect_by_number,
    olc_parse_affect_value_fn parse_affect_value,
    olc_affect_list_ref_fn first_affect_ref,
    olc_affect_list_ref_fn last_affect_ref)
{
    if (!ch || !ch->desc || !ch->desc->olc || !target)
        return false;

    std::string arg = ch->desc->olc->last_cmd_arg;
    if (arg.empty())
    {
        olc_show_affect_help(ch);
        return false;
    }

    std::istringstream iss(arg);
    std::string first;
    iss >> first;

    if (first.empty())
    {
        olc_show_affect_help(ch);
        return false;
    }

    AFFECT_DATA*& first_affect = first_affect_ref(target);
    AFFECT_DATA*& last_affect  = last_affect_ref(target);

    if (!str_cmp(first.c_str(), "add"))
    {
        std::string field_name;
        std::string value_text;
        int loc;
        int value = 0;

        iss >> field_name;
        std::getline(iss, value_text);
        value_text = olc_format_trim_left(value_text);

        if (field_name.empty())
        {
            send_to_char("Add which affect location?\n", ch);
            olc_show_affect_location_choices(ch);
            return false;
        }

        loc = get_atype(const_cast<char*>(field_name.c_str()));
        if (loc < 1)
        {
            ch_printf(ch, "Unknown affect field: %s\n", field_name.c_str());
            olc_show_affect_location_choices(ch);
            return false;
        }

        if (value_text.empty())
        {
            olc_show_affect_modifier_choices(ch, loc);
            return false;
        }

        if (!parse_affect_value(ch, loc, value_text, value))
        {
            olc_show_affect_modifier_choices(ch, loc);
            return false;
        }

        AFFECT_DATA* af = new AFFECT_DATA{};
        af->type = -1;
        af->duration = -1;
        af->location = loc;
        af->modifier = value;
        af->bitvector = -1;

        af->prev = last_affect;
        af->next = nullptr;

        if (last_affect)
            last_affect->next = af;
        else
            first_affect = af;

        last_affect = af;

        ch->desc->olc->dirty = true;
        send_to_char("Affect added.\n", ch);
        return true;
    }

    if (first[0] != '#')
    {
        olc_show_affect_help(ch);
        return false;
    }

    int index = atoi(first.c_str() + 1);
    AFFECT_DATA* af = find_affect_by_number(target, index);
    if (!af)
    {
        send_to_char("No such affect.\n", ch);
        return false;
    }

    std::string cmd;
    iss >> cmd;

    if (cmd.empty())
    {
        olc_show_affect_help(ch);
        return false;
    }

    if (!str_cmp(cmd.c_str(), "delete"))
    {
        if (af->prev)
            af->prev->next = af->next;
        else
            first_affect = af->next;

        if (af->next)
            af->next->prev = af->prev;
        else
            last_affect = af->prev;

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
            olc_show_affect_location_choices(ch);
            return false;
        }

        int loc = get_atype(const_cast<char*>(field_name.c_str()));
        if (loc < 1)
        {
            ch_printf(ch, "Unknown affect field: %s\n", field_name.c_str());
            olc_show_affect_location_choices(ch);
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
            olc_show_affect_modifier_choices(ch, af->location);
            return false;
        }

        if (!parse_affect_value(ch, af->location, value_text, value))
        {
            olc_show_affect_modifier_choices(ch, af->location);
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
            send_to_char("Must be a number >= -1 \n", ch);
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
        int value = -1;
        const flag_name* flag;
        std::string value_text;

        std::getline(iss, value_text);
        value_text = olc_format_trim_left(value_text);

        flag = find_flag(aff_flags, value_text);
        if (flag)
            value = flag->bit;

        if (value_text.empty() || value == -1)
        {
            send_to_char("Set bitvector to what?\n", ch);
            std::vector<std::string> values;
            for (size_t i = 0; i < AFF_MAX; ++i)
                values.push_back(aff_flags[i].name);

            if (!values.empty())
            {
                send_to_char("Valid values:\n", ch);
                std::string col = olc_format_columns(
                    values,
                    ch->desc && ch->desc->term_width > 20 ? ch->desc->term_width : 75,
                    4);
                send_to_char(col.c_str(), ch);
            }

            return false;
        }

        af->bitvector = value;
        ch->desc->olc->dirty = true;
        send_to_char("Affect bitvector updated.\n", ch);
        return true;
    }

    olc_show_affect_help(ch);
    return false;
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

void olc_apply_flag_delta(BitSet& dst, const BitSet& edited, const BitSet& baseline)
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

CMDTYPE* olc_find_command_exact_for_char(CHAR_DATA* ch, const char* command)
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

bool olc_set_dirty(CHAR_DATA *ch)
{
    if (ch && ch->desc && ch->desc->olc)
    {
    	ch->desc->olc->dirty = true;
        return true;
    }
    return false;
}

bool olc_can_use_command(CHAR_DATA* ch, const char* command)
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

static bool olc_finish_editor_substate(CHAR_DATA* ch)
{
    if (!ch || !ch->desc || !ch->desc->olc)
        return false;

    auto sess = ch->desc->olc;
    if (!sess || !sess->working_copy)
        return false;

    switch (sess->mode)
    {
        case OlcEditMode::ROOM_INLINE:
            return olc_finish_editor_substate_typed(
                ch, sess, get_room_schema(),
                olc_session_working_as<ROOM_INDEX_DATA>(sess));

        case OlcEditMode::OBJECT_INLINE:
            return olc_finish_editor_substate_typed(
                ch, sess, get_object_schema(),
                olc_session_working_as<OBJ_DATA>(sess));

        case OlcEditMode::MOBILE_INLINE:
            return olc_finish_editor_substate_typed(
                ch, sess, get_mobile_schema(),
                olc_session_working_as<CHAR_DATA>(sess));

        case OlcEditMode::SHOP_INLINE:
            return olc_finish_editor_substate_typed(
                ch, sess, get_shop_schema(),
                olc_session_working_as<SHOP_DATA>(sess));

        default:
            return false;
    }
}

bool olc_has_active_session(CHAR_DATA* ch)
{
    return ch && ch->desc && ch->desc->olc;
}

bool olc_matches_word(const std::string& input, const char* word)
{
    return !str_prefix(input.c_str(), word);
}

bool olc_dispatch_entry_command(CHAR_DATA* ch, const std::string& type, const std::string& rest)
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

        if (olc_matches_word(type, "shop") || olc_matches_word(type, "sho") || olc_matches_word(type, "s"))
    {
        if (!olc_can_use_command(ch, "shopset"))
        {
            send_to_char("You do not have permission to edit shops (shopset)\n", ch);
            return true;
        }

        if (rest.empty())
        {
            send_to_char("Usage: olc shop <shop>\n", ch);
            return true;
        }

        snprintf(buf, sizeof(buf), "%s", rest.c_str());
        do_shopset(ch, buf);
        return true;
    }

    return false;
}

bool olc_session_command_is_field_name(OlcSession* sess, const std::string& cmd)
{
    if (!sess || cmd.empty())
        return false;

    switch (sess->mode)
    {
        case OlcEditMode::ROOM_INLINE:
            return olc_schema_has_field_name(get_room_schema(), cmd);

        case OlcEditMode::OBJECT_INLINE:
            return olc_schema_has_field_name(get_object_schema(), cmd);

        case OlcEditMode::MOBILE_INLINE:
            return olc_schema_has_field_name(get_mobile_schema(), cmd);

        case OlcEditMode::SHOP_INLINE:
            return olc_schema_has_field_name(get_shop_schema(), cmd);

        default:
            return false;
    }
}

bool olc_dispatch_session_command(CHAR_DATA* ch, const std::string& cmd, const std::string& rest)
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

        if (!str_cmp(sess->schema->name, "shop"))
        {
            olc_shop_edit_revert(ch);
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
    if (olc_session_command_is_field_name(sess, cmd))
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
// -------GENERIC----------

void olc_show_extradesc_help(CHAR_DATA* ch)
{
    send_to_char("Usage:\n", ch);
    send_to_char("  extradesc <keyword>     (edit/create)\n", ch);
    send_to_char("  extradesc -<keyword>    (delete)\n", ch);
}

void olc_show_main_help(CHAR_DATA* ch)
{
    send_to_char("OLC commands:\n", ch);
    send_to_char("  olc room              - edit current room\n", ch);
    send_to_char("  olc object <target>   - edit object by keyword/vnum\n", ch);
    send_to_char("  olc mobile <target>   - edit mobile by keyword/vnum\n", ch);
    send_to_char("  olc shop <target>   - edit mobile's shop by vnum\n", ch);
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

void olc_show_help(CHAR_DATA* ch, const std::string& field)
{
    if (!ch || !ch->desc || !ch->desc->olc)
    {
        send_to_char("You are not editing anything.\n", ch);
        return;
    }

    OlcSession* sess = ch->desc->olc;

    switch (sess->mode)
    {
        case OlcEditMode::ROOM_INLINE:
            olc_show_help_typed(ch, get_room_schema(), field);
            return;

        case OlcEditMode::OBJECT_INLINE:
            olc_show_help_typed(ch, get_object_schema(), field);
            return;

        case OlcEditMode::MOBILE_INLINE:
            olc_show_help_typed(ch, get_mobile_schema(), field);
            return;

        default:
            send_to_char("You are not editing anything.\n", ch);
            return;
    }
}

void olc_show(CHAR_DATA* ch, const std::string& field, const std::string& value)
{
    if (!ch || !ch->desc || !ch->desc->olc)
    {
        send_to_char("You are not editing anything.\n", ch);
        return;
    }

    OlcSession* sess = ch->desc->olc;
    int term_width = (ch->desc->term_width > 0) ? ch->desc->term_width : 80;

    switch (sess->mode)
    {
        case OlcEditMode::ROOM_INLINE:
            olc_show_typed(
                ch,
                sess,
                olc_session_working_as<ROOM_INDEX_DATA>(sess),
                get_room_schema(),
                field,
                value,
                term_width);
            return;

        case OlcEditMode::OBJECT_INLINE:
            olc_show_typed(
                ch,
                sess,
                olc_session_working_as<OBJ_DATA>(sess),
                get_object_schema(),
                field,
                value,
                term_width);
            return;

        case OlcEditMode::MOBILE_INLINE:
            olc_show_typed(
                ch,
                sess,
                olc_session_working_as<CHAR_DATA>(sess),
                get_mobile_schema(),
                field,
                value,
                term_width);
            return;
        case OlcEditMode::SHOP_INLINE:
            olc_show_typed(
                ch,
                sess,
                olc_session_working_as<SHOP_DATA>(sess),
                get_shop_schema(),
                field,
                value,
                term_width);
            return;

        default:
            send_to_char("You are not editing anything.\n", ch);
            return;
    }
}



// --------------------------------------------
// Session Control OLC_START OLC_STOP OLC_SET
// --------------------------------------------
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
    auto d = ch ? ch->desc : nullptr;
    if (!d || !d->olc)
        return;

    auto sess = d->olc;

    switch (sess->mode)
    {
        case OlcEditMode::ROOM_INLINE:
            olc_set_typed(
                ch, sess,
                olc_session_working_as<ROOM_INDEX_DATA>(sess),
                get_room_schema(),
                field, value);
            return;

        case OlcEditMode::OBJECT_INLINE:
            olc_set_typed(
                ch, sess,
                olc_session_working_as<OBJ_DATA>(sess),
                get_object_schema(),
                field, value);
            return;

        case OlcEditMode::MOBILE_INLINE:
            olc_set_typed(
                ch, sess,
                olc_session_working_as<CHAR_DATA>(sess),
                get_mobile_schema(),
                field, value);
            return;

        case OlcEditMode::SHOP_INLINE:
            olc_set_typed(
                ch, sess,
                olc_session_working_as<SHOP_DATA>(sess),
                get_shop_schema(),
                field, value);
            return;


        default:
            send_to_char("You are not editing anything.\n", ch);
            return;
    }
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