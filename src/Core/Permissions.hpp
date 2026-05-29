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
    Kick = 'c',
    Ban = 'd',
    Unban = 'e',
    VoiceMute = 'o',
    TextMute = 'p',
    Warn = 'q',
    Control = 's',  // slay / move / teleport / freeze / noclip / bury / team
    Fun = 'f',      // ghost / disco / smite
    Health = 'h',   // health / armor / godmode
    Hide = 'b',
    CheatCheck = 'k',
    Root = 'z',
};

/** The single-character flag string for a permission (for CommandBuilder::RequirePermission). */
inline std::string Flag(Permission p)
{
    return std::string(1, static_cast<char>(p));
}

}  // namespace AdminSystem
