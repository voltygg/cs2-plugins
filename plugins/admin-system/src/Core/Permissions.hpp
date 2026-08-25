#pragma once

#include <string>

namespace AdminSystem
{

/**
 * Admin permission flags. The underlying char is the flag letter stored in the DB
 * (`admins.flags` / `admin_groups.flags`); 'z' (Root) grants everything. Use these instead of
 * bare character literals at permission-check and command-registration sites.
 */
enum class Permission : char
{
    FreezeAdmins = 'a',  // freeze/unfreeze other admins' privileges, list frozen admins
    Kick = 'c',
    Ban = 'd',
    Unban = 'e',
    Mute = 'o',     // voice mute / text mute / warn
    Control = 's',  // slay / move / teleport / freeze / noclip / bury / team / speed
    Fun = 'f',      // ghost / disco / smite / size
    Health = 'h',   // health / armor / godmode
    Hide = 'b',
    Wallhack = 'w',  // grant a target see-through-walls glow vision
    Bhop = 'j',      // grant a target session bunnyhop (requires the bhop plugin, "grants" mode)
    Map = 'm',       // change level, queue the next map
    Weapon = 'k',    // give and strip weapons
    FunMode = 'g',   // server-wide round modifiers
    Root = 'z',
};

/** The single-character flag string for a permission (for CommandSpec::Permission). */
inline std::string Flag(Permission p)
{
    return std::string(1, static_cast<char>(p));
}

}  // namespace AdminSystem
