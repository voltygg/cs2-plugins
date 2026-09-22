#pragma once

namespace AdminSystem::Permission
{

/**
 * Permission names stored in `admins.permissions` / `admin_groups.permissions` (JSON arrays).
 * A grant of "*" holds every permission, and "admin.*" every name under "admin.".
 */
inline constexpr const char* FreezeAdmins = "admin.freeze_admins";  // freeze other admins' privileges
inline constexpr const char* Kick = "admin.kick";
inline constexpr const char* Ban = "admin.ban";
inline constexpr const char* Unban = "admin.unban";
inline constexpr const char* Mute = "admin.mute";  // voice mute / text mute / warn
inline constexpr const char* Control =
    "admin.control";                                   // slay / move / teleport / freeze / noclip / bury / team / speed
inline constexpr const char* Fun = "admin.fun";        // ghost / disco / smite / size
inline constexpr const char* Health = "admin.health";  // health / armor / godmode
inline constexpr const char* Hide = "admin.hide";
inline constexpr const char* Wallhack = "admin.wallhack";
inline constexpr const char* Bhop = "admin.bhop";  // requires the bhop plugin in "grants" mode
inline constexpr const char* Map = "admin.map";
inline constexpr const char* Weapon = "admin.weapon";
inline constexpr const char* FunMode = "admin.fun_mode";  // server-wide round modifiers
inline constexpr const char* Vote = "admin.vote";
inline constexpr const char* Root = "*";

}  // namespace AdminSystem::Permission
